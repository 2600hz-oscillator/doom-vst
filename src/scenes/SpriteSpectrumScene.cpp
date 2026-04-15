#include "SpriteSpectrumScene.h"
#include "doom/DoomEngine.h"
#include "doom_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

SpriteSpectrumScene::SpriteSpectrumScene()
    : rgbaBuffer(static_cast<size_t>(kWidth * kHeight * 4), 0)
{
}

void SpriteSpectrumScene::init(DoomEngine& engine)
{
    bandAmplitudes.fill(0.0f);
    backgroundBrightness = 0.0f;
    loadPalette(engine);
}

void SpriteSpectrumScene::loadPalette(DoomEngine& engine)
{
    if (!paletteLoaded && engine.isInitialized())
    {
        if (engine.isMapLoaded())
        {
            doom_frame_t* frame = doom_render_frame();
            if (frame)
            {
                std::memcpy(palette, frame->palette, 768);
                paletteLoaded = true;
            }
        }
    }
}

void SpriteSpectrumScene::update(DoomEngine& engine, const ParameterMap& params,
                                  float deltaTime)
{
    (void)engine;
    (void)deltaTime;

    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        std::string key = "band." + std::to_string(i) + ".amplitude";
        auto it = params.find(key);
        if (it != params.end())
            bandAmplitudes[static_cast<size_t>(i)] = it->second;
    }

    // Fallback: distribute overall level across bands
    auto lit = params.find("sector_light.all");
    if (lit != params.end())
    {
        for (int i = 0; i < kNumDisplayBands; ++i)
        {
            if (bandAmplitudes[static_cast<size_t>(i)] == 0.0f)
            {
                float variation = 0.5f + 0.5f * std::sin(static_cast<float>(i) * 0.8f);
                bandAmplitudes[static_cast<size_t>(i)] = lit->second * variation;
            }
        }
    }

    auto bit = params.find("background_brightness");
    if (bit != params.end())
        backgroundBrightness = bit->second;
}

void SpriteSpectrumScene::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) return;
    int idx = (y * kWidth + x) * 4;
    rgbaBuffer[static_cast<size_t>(idx + 0)] = r;
    rgbaBuffer[static_cast<size_t>(idx + 1)] = g;
    rgbaBuffer[static_cast<size_t>(idx + 2)] = b;
    rgbaBuffer[static_cast<size_t>(idx + 3)] = 255;
}

void SpriteSpectrumScene::clearBuffer()
{
    uint8_t bg = static_cast<uint8_t>(backgroundBrightness * 30.0f);
    for (int i = 0; i < kWidth * kHeight; ++i)
    {
        int idx = i * 4;
        rgbaBuffer[static_cast<size_t>(idx + 0)] = bg;
        rgbaBuffer[static_cast<size_t>(idx + 1)] = bg;
        rgbaBuffer[static_cast<size_t>(idx + 2)] = static_cast<uint8_t>(bg + bg / 4);
        rgbaBuffer[static_cast<size_t>(idx + 3)] = 255;
    }
}

// Decode a Doom patch_t and draw it scaled at the given position.
// patchData points to the raw patch_t lump data.
void SpriteSpectrumScene::drawPatchSprite(const uint8_t* patchData, int centerX,
                                           int baseY, float scale)
{
    if (!patchData || !paletteLoaded || scale < 0.01f) return;

    // Read patch header (little-endian)
    int16_t width = static_cast<int16_t>(patchData[0] | (patchData[1] << 8));
    int16_t height = static_cast<int16_t>(patchData[2] | (patchData[3] << 8));
    int16_t leftoff = static_cast<int16_t>(patchData[4] | (patchData[5] << 8));
    (void)leftoff;

    if (width <= 0 || height <= 0 || width > 256 || height > 256) return;

    int scaledW = std::max(1, static_cast<int>(static_cast<float>(width) * scale));
    int scaledH = std::max(1, static_cast<int>(static_cast<float>(height) * scale));

    int drawLeft = centerX - scaledW / 2;
    int drawTop = baseY - scaledH;

    // Column offsets start at byte 8 (after 4 shorts)
    const uint8_t* colOffsPtr = patchData + 8;

    for (int col = 0; col < width; ++col)
    {
        // Read column offset (4 bytes, little-endian)
        uint32_t colOfs = static_cast<uint32_t>(colOffsPtr[col * 4])
                        | (static_cast<uint32_t>(colOffsPtr[col * 4 + 1]) << 8)
                        | (static_cast<uint32_t>(colOffsPtr[col * 4 + 2]) << 16)
                        | (static_cast<uint32_t>(colOffsPtr[col * 4 + 3]) << 24);

        const uint8_t* colData = patchData + colOfs;

        // Scaled X position for this column
        int sx = drawLeft + static_cast<int>(static_cast<float>(col) * scale);
        int sxEnd = drawLeft + static_cast<int>(static_cast<float>(col + 1) * scale);

        // Parse posts
        while (*colData != 0xFF)
        {
            uint8_t topdelta = colData[0];
            uint8_t length = colData[1];
            // colData[2] is padding
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

                // Fill scaled pixel block
                for (int py = sy; py < syEnd; ++py)
                    for (int px = sx; px < sxEnd; ++px)
                        setPixel(px, py, r, g, b);
            }

            // Skip: topdelta(1) + length(1) + pad(1) + pixels(length) + pad(1)
            colData += 4 + length;
        }
    }
}

const uint8_t* SpriteSpectrumScene::render(DoomEngine& engine)
{
    (void)engine;
    clearBuffer();

    int bandWidth = kWidth / kNumDisplayBands;
    int baseY = kHeight - 10;

    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        float amp = bandAmplitudes[static_cast<size_t>(i)];
        if (amp < 0.01f) continue;

        int centerX = bandWidth / 2 + i * bandWidth;

        // Get the imp sprite (frame 0, rotation 0)
        doom_sprite_t spr = doom_get_sprite(kSpriteId, 0, 0);
        if (spr.patch_data != nullptr)
        {
            float scale = 0.5f + amp * 2.5f; // scale from 0.5x to 3x
            drawPatchSprite(spr.patch_data, centerX, baseY, scale);
        }
    }

    return rgbaBuffer.data();
}

void SpriteSpectrumScene::cleanup(DoomEngine& engine)
{
    (void)engine;
    bandAmplitudes.fill(0.0f);
}
