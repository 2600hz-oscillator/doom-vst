#include "Spectrum2Scene.h"
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "doom_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

static constexpr float TWO_PI = 6.28318530718f;

Spectrum2Scene::Spectrum2Scene(const AudioAnalyzer& az,
                                const patch::VisualizerState& state)
    : analyzer(az),
      vizState(state),
      rgbaBuffer(static_cast<size_t>(kWidth * kHeight * 4), 0)
{
    initSinTable();
    bandGains01.fill(0.625f);
}

void Spectrum2Scene::initSinTable()
{
    for (int i = 0; i < kSinTableSize; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(kSinTableSize) * TWO_PI;
        sinTable[static_cast<size_t>(i)] = std::sin(t);
    }
}

inline float Spectrum2Scene::fastSin(float x) const
{
    // Wrap x to [0, 2*PI) and look up
    float idx = x * static_cast<float>(kSinTableSize) / TWO_PI;
    int i = static_cast<int>(idx) & (kSinTableSize - 1);
    if (i < 0) i += kSinTableSize;
    return sinTable[static_cast<size_t>(i)];
}

void Spectrum2Scene::init(DoomEngine& engine)
{
    // Full reset so revisits via SceneManager::switchTo don't leak prior state
    // (starfield positions, winamp peak meters, doomtex index, etc).
    bandAmplitudes.fill(0.0f);
    bandGains01.fill(0.0f);
    bandSpriteIds.fill(0);
    overallRMS = 0.0f;
    onsetTrigger = false;
    time = 0.0f;
    paletteShift = 0.0f;

    doomtexIndex = 0;
    doomtexAboveThreshold = false;
    doomtexCachedIndex = -1;
    doomtexPixels.clear();

    starsInitialized = false;
    winampPeak.fill(0.0f);

    currentVibe = patch::BackgroundVibe::AcidwarpExe;

    loadPalette(engine);
}

void Spectrum2Scene::loadPalette(DoomEngine& engine)
{
    if (!paletteLoaded && engine.isInitialized() && engine.isMapLoaded())
    {
        doom_frame_t* frame = doom_render_frame();
        if (frame)
        {
            std::memcpy(palette, frame->palette, 768);
            paletteLoaded = true;
        }
    }
}

void Spectrum2Scene::update(DoomEngine& engine, const ParameterMap& params, float deltaTime)
{
    (void)engine;
    lastDelta = deltaTime;

    // Pull current state snapshots under SpinLock. Bands live in GlobalConfig
    // (shared across scenes); the active background vibe is per-scene.
    patch::GlobalConfig   global = vizState.getGlobal();
    patch::SpectrumConfig spec   = vizState.getSpectrum();
    currentVibe = spec.vibe;

    // Compute per-band amplitudes from the raw FFT magnitude spectrum using
    // the user-configured Hz ranges. Replaces the prior fixed 8-from-16
    // mapping so the UI's lowHz/highHz textboxes drive band membership.
    const float* spectrum = analyzer.getMagnitudeSpectrum();
    const int specSize = analyzer.getSpectrumSize();
    const double sampleRate = analyzer.getSampleRate();
    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(AudioAnalyzer::kFFTSize);

    std::array<float, kNumDisplayBands> rawBand {};
    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        const auto& cfg = global.bands[static_cast<size_t>(i)];
        bandGains01[static_cast<size_t>(i)] = cfg.gain01;
        bandSpriteIds[static_cast<size_t>(i)] = cfg.spriteId;

        int lowBin = std::max(1, static_cast<int>(cfg.lowHz / binWidth));
        int highBin = std::min(specSize - 1, static_cast<int>(cfg.highHz / binWidth));
        if (highBin < lowBin) std::swap(lowBin, highBin);

        float sumSq = 0.0f;
        int count = 0;
        for (int bin = lowBin; bin <= highBin; ++bin)
        {
            float mag = spectrum[static_cast<size_t>(bin)];
            sumSq += mag * mag;
            count++;
        }
        rawBand[static_cast<size_t>(i)] = count > 0 ? std::sqrt(sumSq / static_cast<float>(count)) : 0.0f;
    }

    // Peak-normalize across our 8 bands (mirrors AudioAnalyzer's approach so
    // the loudest band lands at ~1.0 and relative balance is preserved).
    float maxBand = *std::max_element(rawBand.begin(), rawBand.end());
    if (maxBand > 0.0001f)
    {
        for (auto& b : rawBand)
            b = std::min(1.0f, b / maxBand);
    }

    // Per-frame envelope follower (one-pole, attack/release time constants).
    constexpr float kAttack = 0.01f;
    constexpr float kRelease = 0.15f;
    float dt = std::max(0.001f, deltaTime);
    float attackCoeff = 1.0f - std::exp(-dt / kAttack);
    float releaseCoeff = 1.0f - std::exp(-dt / kRelease);
    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        float target = rawBand[static_cast<size_t>(i)];
        float& env = bandAmplitudes[static_cast<size_t>(i)];
        env += (target > env ? attackCoeff : releaseCoeff) * (target - env);
    }

    overallRMS = analyzer.getRMSLevel();
    onsetTrigger = analyzer.getOnset();

    // DOOMTEX manual override: when the user picks a new texture in the
    // dropdown and clicks Apply, the stored doomtexIndex changes — snap the
    // live index to it. Tracking the last-seen value avoids treating our own
    // auto-advance as an external override.
    if (spec.doomtexIndex != lastSeenStoredDoomtexIndex)
    {
        lastSeenStoredDoomtexIndex = spec.doomtexIndex;
        doomtexIndex = juce::jlimit(0, 7, spec.doomtexIndex);
        doomtexAboveThreshold = false;  // reset edge detector
    }

    // DOOMTEX auto-advance: cycle to the next texture each time the user-
    // configured band crosses the threshold (rising edge with hysteresis
    // computed as `threshold - 0.1` to match the previous kHi/kLo spread).
    if (currentVibe == patch::BackgroundVibe::Doomtex && spec.doomtexAutoAdvance)
    {
        int bandIdx = juce::jlimit(0, kNumDisplayBands - 1, spec.doomtexAutoBand - 1);
        float bandAmp = bandAmplitudes[static_cast<size_t>(bandIdx)];
        float kHi = juce::jlimit(0.0f, 1.0f, spec.doomtexAutoThreshold);
        float kLo = std::max(0.0f, kHi - 0.1f);
        if (! doomtexAboveThreshold && bandAmp > kHi)
        {
            doomtexAboveThreshold = true;
            doomtexIndex = (doomtexIndex + 1) % 8;  // matches kDoomtexNames len
        }
        else if (doomtexAboveThreshold && bandAmp < kLo)
        {
            doomtexAboveThreshold = false;
        }
    }

    // Time advances with high-freq energy (cycle speed)
    float highEnergy = 0.5f * (bandAmplitudes[6] + bandAmplitudes[7]);
    float timeRate = 0.3f + highEnergy * 4.0f;
    time += timeRate * deltaTime;

    // Palette shift jolts on onsets, drifts otherwise
    if (onsetTrigger) paletteShift += 16.0f;
    paletteShift += deltaTime * 8.0f;
    if (paletteShift > 256.0f) paletteShift -= 256.0f;

    // Read params (allow YAML override)
    auto it = params.find("background_brightness");
    if (it != params.end())
        overallRMS = std::max(overallRMS, it->second);
}

void Spectrum2Scene::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return;
    int idx = (y * kWidth + x) * 4;
    rgbaBuffer[static_cast<size_t>(idx + 0)] = r;
    rgbaBuffer[static_cast<size_t>(idx + 1)] = g;
    rgbaBuffer[static_cast<size_t>(idx + 2)] = b;
    rgbaBuffer[static_cast<size_t>(idx + 3)] = 255;
}

void Spectrum2Scene::renderAcidwarpBackground()
{
    if (!paletteLoaded) return;

    // Audio-driven parameters
    float a = 0.04f + bandAmplitudes[0] * 0.20f;        // sub-bass: horizontal stripe freq
    float b = 0.05f + bandAmplitudes[1] * 0.20f;        // bass: vertical stripe freq
    float c = 0.03f + 0.5f * (bandAmplitudes[2] + bandAmplitudes[3]) * 0.15f;  // low-mid: radial freq
    float d = 0.04f + 0.5f * (bandAmplitudes[4] + bandAmplitudes[5]) * 0.15f;  // mid/upper-mid: diagonal freq

    float intensity = 0.3f + overallRMS * 4.0f;
    if (intensity > 1.5f) intensity = 1.5f;

    float cx = static_cast<float>(kWidth) * 0.5f;
    float cy = static_cast<float>(kHeight) * 0.5f;
    float palOff = paletteShift;

    // Render at half resolution then expand 2x for performance
    constexpr int kStep = 2;
    for (int y = 0; y < kHeight; y += kStep)
    {
        float fy = static_cast<float>(y);
        for (int x = 0; x < kWidth; x += kStep)
        {
            float fx = static_cast<float>(x);

            float dx = fx - cx;
            float dy = fy - cy;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Layered sin functions
            float v = fastSin(fx * a + time)
                    + fastSin(fy * b + time * 0.7f)
                    + fastSin(dist * c - time * 1.3f)
                    + fastSin((fx + fy) * d + time * 0.5f);

            // Map to palette index — saturated color ranges
            // Doom palette colorful zones: greens 112-127, reds 168-191, blues/purples 200-207, oranges 215-231
            float scaled = (v + 4.0f) * 16.0f * intensity + palOff;
            int palIdx = (static_cast<int>(scaled)) & 0xFF;

            // Bias toward saturated colors (skip muddy browns 16-79)
            // Map 0-255 -> selected colorful ranges
            int range = palIdx % 4;
            int offsetInRange = (palIdx / 4) % 16;
            int finalIdx;
            switch (range)
            {
                case 0: finalIdx = 112 + offsetInRange; break;       // greens
                case 1: finalIdx = 168 + offsetInRange + 8; break;   // reds (brighter range)
                case 2: finalIdx = 192 + offsetInRange % 8; break;   // blues
                default: finalIdx = 215 + offsetInRange; break;      // oranges
            }
            finalIdx &= 0xFF;

            // Dim background so sprites stand out (35% brightness)
            constexpr float kBgBrightness = 0.35f;
            uint8_t r = static_cast<uint8_t>(palette[finalIdx * 3 + 0] * kBgBrightness);
            uint8_t g = static_cast<uint8_t>(palette[finalIdx * 3 + 1] * kBgBrightness);
            uint8_t bb = static_cast<uint8_t>(palette[finalIdx * 3 + 2] * kBgBrightness);

            // 2x2 block fill
            for (int dyy = 0; dyy < kStep; ++dyy)
                for (int dxx = 0; dxx < kStep; ++dxx)
                    setPixel(x + dxx, y + dyy, r, g, bb);
        }
    }
}

void Spectrum2Scene::drawPatchSprite(const uint8_t* patchData, int centerX,
                                      int baseY, float scale)
{
    if (!patchData || !paletteLoaded || scale < 0.01f) return;

    int16_t width = static_cast<int16_t>(patchData[0] | (patchData[1] << 8));
    int16_t height = static_cast<int16_t>(patchData[2] | (patchData[3] << 8));

    if (width <= 0 || height <= 0 || width > 320 || height > 256) return;

    int scaledW = std::max(1, static_cast<int>(static_cast<float>(width) * scale));
    int scaledH = std::max(1, static_cast<int>(static_cast<float>(height) * scale));

    int drawLeft = centerX - scaledW / 2;
    int drawTop = baseY - scaledH;

    const uint8_t* colOffsPtr = patchData + 8;

    for (int col = 0; col < width; ++col)
    {
        uint32_t colOfs = static_cast<uint32_t>(colOffsPtr[col * 4])
                        | (static_cast<uint32_t>(colOffsPtr[col * 4 + 1]) << 8)
                        | (static_cast<uint32_t>(colOffsPtr[col * 4 + 2]) << 16)
                        | (static_cast<uint32_t>(colOffsPtr[col * 4 + 3]) << 24);

        const uint8_t* colData = patchData + colOfs;

        int sx = drawLeft + static_cast<int>(static_cast<float>(col) * scale);
        int sxEnd = drawLeft + static_cast<int>(static_cast<float>(col + 1) * scale);

        while (*colData != 0xFF)
        {
            uint8_t topdelta = colData[0];
            uint8_t length = colData[1];
            const uint8_t* pixels = colData + 3;

            for (int row = 0; row < length; ++row)
            {
                uint8_t palIdx = pixels[row];
                uint8_t r = palette[palIdx * 3 + 0];
                uint8_t g = palette[palIdx * 3 + 1];
                uint8_t b = palette[palIdx * 3 + 2];

                int srcY = topdelta + row;
                int sy = drawTop + static_cast<int>(static_cast<float>(srcY) * scale);
                int syEnd = drawTop + static_cast<int>(static_cast<float>(srcY + 1) * scale);

                for (int py = sy; py < syEnd; ++py)
                    for (int px = sx; px < sxEnd; ++px)
                        setPixel(px, py, r, g, b);
            }

            colData += 4 + length;
        }
    }
}

const uint8_t* Spectrum2Scene::render(DoomEngine& engine)
{
    (void)engine;

    renderBackground(currentVibe);

    // Draw sprites on top of background
    int bandWidth = kWidth / kNumDisplayBands;
    int baseY = kHeight - 10;

    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        float amp = bandAmplitudes[static_cast<size_t>(i)];
        float gain01 = bandGains01[static_cast<size_t>(i)];
        // Slider all the way left → no contribution → sprite hidden.
        if (gain01 < 0.001f) continue;
        if (amp < 0.01f) continue;

        int centerX = bandWidth / 2 + i * bandWidth;
        int spriteId = bandSpriteIds[static_cast<size_t>(i)];

        doom_sprite_t spr = doom_get_sprite(spriteId, 0, 0);
        if (spr.patch_data != nullptr)
        {
            float scale = amp * (gain01 * patch::kSpectrumMaxGain);
            if (scale >= 0.01f)
                drawPatchSprite(spr.patch_data, centerX, baseY, scale);
        }
    }

    return rgbaBuffer.data();
}

void Spectrum2Scene::cleanup(DoomEngine& engine)
{
    (void)engine;
    bandAmplitudes.fill(0.0f);
}

// ===========================================================================
// Background renderers
// ===========================================================================

void Spectrum2Scene::clearBuffer(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < kWidth * kHeight; ++i)
    {
        rgbaBuffer[static_cast<size_t>(i * 4 + 0)] = r;
        rgbaBuffer[static_cast<size_t>(i * 4 + 1)] = g;
        rgbaBuffer[static_cast<size_t>(i * 4 + 2)] = b;
        rgbaBuffer[static_cast<size_t>(i * 4 + 3)] = 255;
    }
}

void Spectrum2Scene::fillRect(int x0, int y0, int x1, int y1,
                               uint8_t r, uint8_t g, uint8_t b)
{
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > kWidth)  x1 = kWidth;
    if (y1 > kHeight) y1 = kHeight;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            setPixel(x, y, r, g, b);
}

void Spectrum2Scene::renderBackground(patch::BackgroundVibe vibe)
{
    switch (vibe)
    {
        case patch::BackgroundVibe::AcidwarpExe: renderAcidwarpBackground(); break;
        case patch::BackgroundVibe::Vaporwave:   renderVaporwave();          break;
        case patch::BackgroundVibe::Punkrock:    renderPunkrock();           break;
        case patch::BackgroundVibe::Doomtex:     renderDoomtex();            break;
        case patch::BackgroundVibe::Winamp:      renderWinamp();             break;
        case patch::BackgroundVibe::Starfield:   renderStarfield(lastDelta); break;
        case patch::BackgroundVibe::Crtglitch:   renderCrtglitch();          break;
    }
}

// ---------------------------------------------------------------------------
// VAPORWAVE: scrolling vertical bars in purple/teal/pink. Bar width follows
// band 3 (UI-labeled); palette phase animates with band 4. Each bar has a
// vertical gradient (top brighter, bottom darker).
// ---------------------------------------------------------------------------
void Spectrum2Scene::renderVaporwave()
{
    float band3 = bandAmplitudes[2];
    float band4 = bandAmplitudes[3];

    // Bar width: 4..32 px driven by band 3.
    int barW = 4 + static_cast<int>(band3 * 28.0f);
    if (barW < 2) barW = 2;
    float scrollPx = time * 30.0f;  // px / sec
    int scrollOffset = static_cast<int>(scrollPx) % (barW * 3);

    // Three palette colors that animate by hue rotation driven by band 4.
    auto vaporColor = [&](int barIndex, int y) {
        int slot = (barIndex + static_cast<int>(time * (1.0f + band4 * 4.0f))) % 3;
        // Base RGB per slot: purple, teal, pink.
        uint8_t r0, g0, b0;
        switch (slot)
        {
            case 0: r0 = 178; g0 =  64; b0 = 230; break;  // purple
            case 1: r0 =  64; g0 = 220; b0 = 220; break;  // teal
            default: r0 = 255; g0 =  80; b0 = 180; break; // pink
        }
        // Vertical gradient: top 100% → bottom 30%.
        float v = 1.0f - (static_cast<float>(y) / static_cast<float>(kHeight)) * 0.7f;
        return std::array<uint8_t, 3>{
            static_cast<uint8_t>(r0 * v),
            static_cast<uint8_t>(g0 * v),
            static_cast<uint8_t>(b0 * v),
        };
    };

    for (int x = 0; x < kWidth; ++x)
    {
        int shifted = (x + scrollOffset) % (barW * 1024 * 1024);
        int barIndex = shifted / barW;
        for (int y = 0; y < kHeight; ++y)
        {
            auto c = vaporColor(barIndex, y);
            setPixel(x, y, c[0], c[1], c[2]);
        }
    }

    // Horizon line — subtle pink scanline ~mid-screen, common 80s/90s aesthetic.
    int horizonY = kHeight / 2;
    for (int x = 0; x < kWidth; ++x)
        setPixel(x, horizonY, 255, 200, 230);
}

// ---------------------------------------------------------------------------
// PUNKROCK: grid of brown / black / gray squares. Cell size scales with
// band 3. Pseudo-random palette per cell stays stable; cells "shake" with
// per-cell jitter driven by overall RMS.
// ---------------------------------------------------------------------------
void Spectrum2Scene::renderPunkrock()
{
    float band3 = bandAmplitudes[2];
    int cell = 6 + static_cast<int>(band3 * 22.0f);  // 6..28 px
    if (cell < 4) cell = 4;

    // Background is dirty black.
    clearBuffer(15, 12, 10);

    int cellsX = (kWidth + cell - 1) / cell;
    int cellsY = (kHeight + cell - 1) / cell;

    for (int cy = 0; cy < cellsY; ++cy)
    {
        for (int cx = 0; cx < cellsX; ++cx)
        {
            // Stable hash so the pattern doesn't strobe; modulate slowly with time.
            uint32_t h = static_cast<uint32_t>(cx * 73856093) ^
                         static_cast<uint32_t>(cy * 19349663) ^
                         static_cast<uint32_t>(static_cast<int>(time * 1.3f) * 83492791);
            int sel = h % 6;
            uint8_t r, g, b;
            switch (sel)
            {
                case 0: r =  20; g =  18; b =  16; break; // black
                case 1: r =  60; g =  45; b =  32; break; // dark brown
                case 2: r = 110; g =  80; b =  50; break; // brown
                case 3: r =  90; g =  90; b =  90; break; // gray
                case 4: r = 140; g = 140; b = 140; break; // light gray
                default: r = 200; g =  40; b =  40; break; // splash of red
            }

            // Jitter offset: small per-cell shake on loud signal.
            int jitter = static_cast<int>(overallRMS * 4.0f);
            int jx = (h >> 4) % (jitter + 1) - jitter / 2;
            int jy = (h >> 8) % (jitter + 1) - jitter / 2;

            int x0 = cx * cell + jx;
            int y0 = cy * cell + jy;
            int x1 = x0 + cell - 1;  // -1 leaves a 1px black gutter
            int y1 = y0 + cell - 1;
            fillRect(x0, y0, x1, y1, r, g, b);
        }
    }
}

// ---------------------------------------------------------------------------
// DOOMTEX: tiles a wall texture from E1M1 across the screen. doomtexIndex
// advances on band 3 rising-edge through 50% (handled in update()).
// ---------------------------------------------------------------------------
namespace
{
    // E1M1 wall textures present in shareware DOOM1.WAD.
    constexpr const char* kDoomtexNames[8] = {
        "STARTAN3", "BROWN1",  "COMPSTA1", "COMPUTE1",
        "TEKWALL1", "BROWNHUG", "BIGDOOR2", "METAL"
    };
}

extern "C" int doom_get_wall_texture_data(const char* tex_name, uint8_t* out_pixels,
                                            int max_pixels, int* out_width, int* out_height);

void Spectrum2Scene::cacheDoomtex(int idx)
{
    if (idx == doomtexCachedIndex && ! doomtexPixels.empty())
        return;

    int w = 0, h = 0;
    int total = doom_get_wall_texture_data(kDoomtexNames[idx], nullptr, 0, &w, &h);
    if (total <= 0 || w <= 0 || h <= 0)
    {
        doomtexPixels.clear();
        doomtexW = 0;
        doomtexH = 0;
        doomtexCachedIndex = idx;
        return;
    }
    doomtexPixels.resize(static_cast<size_t>(total));
    doom_get_wall_texture_data(kDoomtexNames[idx], doomtexPixels.data(),
                                total, &w, &h);
    doomtexW = w;
    doomtexH = h;
    doomtexCachedIndex = idx;
}

void Spectrum2Scene::renderDoomtex()
{
    if (! paletteLoaded)
    {
        clearBuffer(0, 0, 0);
        return;
    }

    cacheDoomtex(doomtexIndex);
    if (doomtexPixels.empty() || doomtexW <= 0 || doomtexH <= 0)
    {
        clearBuffer(0, 0, 0);
        return;
    }

    // Doom textures are stored column-major: pixel(col,row) = pixels[col*h + row].
    for (int y = 0; y < kHeight; ++y)
    {
        int sy = y % doomtexH;
        for (int x = 0; x < kWidth; ++x)
        {
            int sx = x % doomtexW;
            uint8_t palIdx = doomtexPixels[static_cast<size_t>(sx * doomtexH + sy)];
            uint8_t r = palette[palIdx * 3 + 0];
            uint8_t g = palette[palIdx * 3 + 1];
            uint8_t b = palette[palIdx * 3 + 2];
            // Dim slightly so sprites stand out.
            setPixel(x, y, static_cast<uint8_t>(r * 0.55f),
                            static_cast<uint8_t>(g * 0.55f),
                            static_cast<uint8_t>(b * 0.55f));
        }
    }
}

// ---------------------------------------------------------------------------
// WINAMP: classic stacked-LED EQ bars. One vertical bar per display band,
// colored green → yellow → red top-to-bottom, with a slow-decay peak hold.
// ---------------------------------------------------------------------------
void Spectrum2Scene::renderWinamp()
{
    clearBuffer(8, 10, 12);

    // Decay peak holds.
    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        float amp = bandAmplitudes[static_cast<size_t>(i)];
        float& peak = winampPeak[static_cast<size_t>(i)];
        if (amp > peak) peak = amp;
        else            peak = std::max(0.0f, peak - 0.6f * lastDelta);
    }

    int marginX = 10;
    int marginYTop = 10;
    int marginYBot = 14;
    int usableW = kWidth - 2 * marginX;
    int usableH = kHeight - marginYTop - marginYBot;

    int slot = usableW / kNumDisplayBands;
    int barW = slot - 4;  // gutters
    if (barW < 4) barW = slot;

    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        float amp = bandAmplitudes[static_cast<size_t>(i)];
        float peak = winampPeak[static_cast<size_t>(i)];
        int barX = marginX + i * slot + (slot - barW) / 2;
        int filledH = static_cast<int>(amp * usableH);
        int peakH   = static_cast<int>(peak * usableH);

        // LED segments: 12 stacked, gap of 1 px between.
        const int kSegments = 12;
        int segH = std::max(2, usableH / kSegments);
        for (int s = 0; s < kSegments; ++s)
        {
            float t = static_cast<float>(s) / static_cast<float>(kSegments - 1);
            // Color: green low, yellow mid, red high.
            uint8_t r, g, b;
            if (t < 0.66f)
            {
                float u = t / 0.66f;
                r = static_cast<uint8_t>(  20 + 200 * u);
                g = static_cast<uint8_t>( 220 -  40 * u);
                b = 30;
            }
            else
            {
                float u = (t - 0.66f) / 0.34f;
                r = static_cast<uint8_t>( 220 +  35 * u);
                g = static_cast<uint8_t>( 180 - 160 * u);
                b = 30;
            }
            int segY1 = marginYTop + usableH - s * segH;
            int segY0 = segY1 - (segH - 1);
            int litThreshold = (s + 1) * segH;
            bool lit = filledH >= litThreshold;
            if (! lit)
            {
                // Dim segment as background frame.
                fillRect(barX, segY0, barX + barW, segY1,
                         static_cast<uint8_t>(r / 8),
                         static_cast<uint8_t>(g / 8),
                         static_cast<uint8_t>(b / 8));
            }
            else
            {
                fillRect(barX, segY0, barX + barW, segY1, r, g, b);
            }
        }

        // Peak hold marker (white sliver at peak position).
        int peakY = marginYTop + usableH - peakH;
        if (peakY >= marginYTop && peakY < marginYTop + usableH)
            fillRect(barX, peakY - 1, barX + barW, peakY + 1, 230, 230, 230);
    }
}

// ---------------------------------------------------------------------------
// STARFIELD: zooming starfield, speed scaled by overall RMS.
// ---------------------------------------------------------------------------
void Spectrum2Scene::initStars()
{
    uint32_t seed = 1u;
    auto rnd = [&]() {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed & 0xffff) / 65535.0f;
    };
    for (auto& s : stars)
    {
        s.x = (rnd() * 2.0f - 1.0f) * 1.0f;
        s.y = (rnd() * 2.0f - 1.0f) * 1.0f;
        s.z = rnd() * 1.0f + 0.05f;
    }
    starsInitialized = true;
}

void Spectrum2Scene::renderStarfield(float deltaTime)
{
    if (! starsInitialized)
        initStars();

    clearBuffer(2, 2, 6);

    float speed = 0.05f + overallRMS * 1.6f;

    for (auto& s : stars)
    {
        s.z -= speed * deltaTime;
        if (s.z <= 0.02f)
        {
            // Recycle far away with new random screen position.
            uint32_t h = static_cast<uint32_t>((s.x + 2.0f) * 12345.6f) ^
                         static_cast<uint32_t>((s.y + 2.0f) * 67890.1f) ^
                         static_cast<uint32_t>(time * 1000.0f);
            float rx = static_cast<float>((h * 1664525u + 1013904223u) & 0xffff) / 65535.0f;
            float ry = static_cast<float>((h * 22695477u + 1u) & 0xffff) / 65535.0f;
            s.x = rx * 2.0f - 1.0f;
            s.y = ry * 2.0f - 1.0f;
            s.z = 1.0f;
        }
        // Project.
        float px = s.x / s.z;
        float py = s.y / s.z;
        int sx = static_cast<int>(px * (kWidth / 2)  + kWidth / 2);
        int sy = static_cast<int>(py * (kHeight / 2) + kHeight / 2);
        if (sx < 0 || sx >= kWidth || sy < 0 || sy >= kHeight) continue;

        // Brighter when closer.
        float bright = std::min(1.0f, 0.3f + (1.0f - s.z));
        uint8_t v = static_cast<uint8_t>(bright * 255.0f);
        setPixel(sx, sy, v, v, v);
        // Cross-hair sparkle for nearby stars.
        if (s.z < 0.25f)
        {
            uint8_t d = static_cast<uint8_t>(v * 0.6f);
            if (sx + 1 < kWidth)  setPixel(sx + 1, sy, d, d, d);
            if (sx - 1 >= 0)      setPixel(sx - 1, sy, d, d, d);
            if (sy + 1 < kHeight) setPixel(sx, sy + 1, d, d, d);
            if (sy - 1 >= 0)      setPixel(sx, sy - 1, d, d, d);
        }
    }
}

// ---------------------------------------------------------------------------
// CRTGLITCH: dark base with horizontal scanlines, occasional tear bands that
// horizontally displace pixels with chromatic aberration. Onsets trigger
// flashes.
// ---------------------------------------------------------------------------
void Spectrum2Scene::renderCrtglitch()
{
    // Base color band scrolls slowly.
    float baseHue = std::fmod(time * 0.05f, 1.0f);
    auto hueColor = [](float h) -> std::array<uint8_t, 3> {
        // Simple HSV -> RGB at S=0.6, V=0.35.
        float s = 0.6f, v = 0.35f;
        float c = v * s;
        float x = c * (1.0f - std::fabs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
        float m = v - c;
        float r=0, g=0, b=0;
        int slot = static_cast<int>(h * 6.0f) % 6;
        switch (slot)
        {
            case 0: r = c; g = x; b = 0; break;
            case 1: r = x; g = c; b = 0; break;
            case 2: r = 0; g = c; b = x; break;
            case 3: r = 0; g = x; b = c; break;
            case 4: r = x; g = 0; b = c; break;
            default: r = c; g = 0; b = x; break;
        }
        return { static_cast<uint8_t>((r + m) * 255),
                 static_cast<uint8_t>((g + m) * 255),
                 static_cast<uint8_t>((b + m) * 255) };
    };
    auto base = hueColor(baseHue);

    // Onset flash boost.
    int flash = onsetTrigger ? 80 : 0;

    for (int y = 0; y < kHeight; ++y)
    {
        // Scanline dim every other row.
        bool scan = (y & 1) == 0;
        uint8_t r = static_cast<uint8_t>(std::min(255, base[0] + flash));
        uint8_t g = static_cast<uint8_t>(std::min(255, base[1] + flash));
        uint8_t b = static_cast<uint8_t>(std::min(255, base[2] + flash));
        if (scan) { r /= 2; g /= 2; b /= 2; }
        for (int x = 0; x < kWidth; ++x)
            setPixel(x, y, r, g, b);
    }

    // Horizontal tear bands.
    int numTears = 3 + static_cast<int>(overallRMS * 8.0f);
    uint32_t seed = static_cast<uint32_t>(time * 6.0f);
    for (int t = 0; t < numTears; ++t)
    {
        seed = seed * 1664525u + 1013904223u;
        int y0 = static_cast<int>(seed % kHeight);
        seed = seed * 1664525u + 1013904223u;
        int h = 1 + static_cast<int>(seed % 6);
        seed = seed * 1664525u + 1013904223u;
        int dx = static_cast<int>(seed % 32) - 16;

        for (int yy = y0; yy < std::min(kHeight, y0 + h); ++yy)
        {
            for (int x = 0; x < kWidth; ++x)
            {
                int sx = x - dx;
                if (sx < 0) sx += kWidth;
                if (sx >= kWidth) sx -= kWidth;
                // Chromatic offsets: red shifted -2, blue +2.
                int rIdx = ((yy * kWidth) + ((sx - 2 + kWidth) % kWidth)) * 4;
                int gIdx = ((yy * kWidth) + sx) * 4;
                int bIdx = ((yy * kWidth) + ((sx + 2) % kWidth)) * 4;
                int dst  = (yy * kWidth + x) * 4;
                rgbaBuffer[static_cast<size_t>(dst + 0)] = rgbaBuffer[static_cast<size_t>(rIdx + 0)];
                rgbaBuffer[static_cast<size_t>(dst + 1)] = rgbaBuffer[static_cast<size_t>(gIdx + 1)];
                rgbaBuffer[static_cast<size_t>(dst + 2)] = rgbaBuffer[static_cast<size_t>(bIdx + 2)];
            }
        }
    }
}
