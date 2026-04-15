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
        // Render one frame to ensure palette is loaded
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

    // Read per-band amplitudes from params
    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        std::string key = "band." + std::to_string(i) + ".amplitude";
        auto it = params.find(key);
        if (it != params.end())
            bandAmplitudes[static_cast<size_t>(i)] = it->second;
    }

    // If no explicit band routing, use sector_light.all as overall level
    auto lit = params.find("sector_light.all");
    if (lit != params.end())
    {
        // Distribute across bands with some variation
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

void SpriteSpectrumScene::drawSprite(int spriteId, int frame, int centerX, int baseY,
                                      float scale)
{
    doom_sprite_t spr = doom_get_sprite(spriteId, frame, 0);
    if (spr.patch_data == nullptr || spr.width == 0 || spr.height == 0)
        return;

    // Scale the sprite height
    int scaledHeight = std::max(1, static_cast<int>(static_cast<float>(spr.height) * scale));
    int scaledWidth = std::max(1, static_cast<int>(static_cast<float>(spr.width) * scale));

    int drawX = centerX - scaledWidth / 2;
    int drawY = baseY - scaledHeight;

    // Simple nearest-neighbor scaling of the sprite
    // Doom patches are column-major with a specific format, but we'll use a simplified approach
    // Just draw a colored rectangle proportional to the sprite with the sprite's dominant color
    // (Full patch decoding is complex — we'll use a solid block with palette color for MVP)

    // Use a color from the Doom palette based on sprite ID
    int palIdx = (spriteId * 16 + 80) % 256;
    uint8_t r = palette[palIdx * 3 + 0];
    uint8_t g = palette[palIdx * 3 + 1];
    uint8_t b = palette[palIdx * 3 + 2];

    // Draw filled rectangle as sprite placeholder
    for (int py = drawY; py < drawY + scaledHeight; ++py)
    {
        for (int px = drawX; px < drawX + scaledWidth; ++px)
        {
            // Add some shading for depth
            float shade = 1.0f - 0.3f * static_cast<float>(py - drawY) / static_cast<float>(scaledHeight);
            setPixel(px, py,
                     static_cast<uint8_t>(static_cast<float>(r) * shade),
                     static_cast<uint8_t>(static_cast<float>(g) * shade),
                     static_cast<uint8_t>(static_cast<float>(b) * shade));
        }
    }
}

const uint8_t* SpriteSpectrumScene::render(DoomEngine& engine)
{
    (void)engine;

    // Clear to dark background
    uint8_t bg = static_cast<uint8_t>(backgroundBrightness * 30.0f);
    for (int i = 0; i < kWidth * kHeight; ++i)
    {
        int idx = i * 4;
        rgbaBuffer[static_cast<size_t>(idx + 0)] = bg;
        rgbaBuffer[static_cast<size_t>(idx + 1)] = bg;
        rgbaBuffer[static_cast<size_t>(idx + 2)] = static_cast<uint8_t>(bg + bg / 4); // slight blue tint
        rgbaBuffer[static_cast<size_t>(idx + 3)] = 255;
    }

    // Draw each band as a sprite
    int bandWidth = kWidth / kNumDisplayBands;
    int baseY = kHeight - 10; // bottom margin

    for (int i = 0; i < kNumDisplayBands; ++i)
    {
        float amp = bandAmplitudes[static_cast<size_t>(i)];
        if (amp < 0.01f) continue;

        int centerX = bandWidth / 2 + i * bandWidth;
        float scale = amp * 3.0f; // scale sprite up to 3x based on amplitude

        drawSprite(kBandSprites[i], 0, centerX, baseY, scale);
    }

    return rgbaBuffer.data();
}

void SpriteSpectrumScene::cleanup(DoomEngine& engine)
{
    (void)engine;
    bandAmplitudes.fill(0.0f);
}
