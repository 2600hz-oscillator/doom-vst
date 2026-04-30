#include "Spectrum2Scene.h"
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "doom_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

static constexpr float TWO_PI = 6.28318530718f;

Spectrum2Scene::Spectrum2Scene(const AudioAnalyzer& az)
    : analyzer(az),
      rgbaBuffer(static_cast<size_t>(kWidth * kHeight * 4), 0)
{
    initSinTable();
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
    bandAmplitudes.fill(0.0f);
    overallRMS = 0.0f;
    onsetTrigger = false;
    time = 0.0f;
    paletteShift = 0.0f;
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

    // Time mult driven by high frequencies (bands 6,7 — highs/air)
    const float* envelope = analyzer.getBandEnvelope();
    int analyzerBands = analyzer.getNumBands();

    // Map analyzer bands to our 8 display bands
    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        int srcIdx = i * analyzerBands / kNumDisplayBands;
        int srcEnd = (i + 1) * analyzerBands / kNumDisplayBands;
        float sum = 0.0f;
        int count = 0;
        for (int j = srcIdx; j < srcEnd && j < analyzerBands; ++j)
        {
            sum += envelope[j];
            count++;
        }
        bandAmplitudes[static_cast<size_t>(i)] = count > 0 ? sum / static_cast<float>(count) : 0.0f;
    }

    overallRMS = analyzer.getRMSLevel();
    onsetTrigger = analyzer.getOnset();

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

    renderAcidwarpBackground();

    // Draw sprites on top of background
    int bandWidth = kWidth / kNumDisplayBands;
    int baseY = kHeight - 10;

    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        float amp = bandAmplitudes[static_cast<size_t>(i)];
        if (amp < 0.01f) continue;

        int centerX = bandWidth / 2 + i * bandWidth;
        int spriteId = kBandSprites[i];

        doom_sprite_t spr = doom_get_sprite(spriteId, 0, 0);
        if (spr.patch_data != nullptr)
        {
            float scale = 0.5f + amp * 2.5f;
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
