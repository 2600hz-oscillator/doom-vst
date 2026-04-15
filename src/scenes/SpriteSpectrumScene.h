#pragma once

#include "Scene.h"
#include <vector>
#include <cstdint>
#include <array>

// Scene B: 2D sprite spectrum visualizer.
// Doom sprites (imps etc.) are drawn as frequency band columns, scaled by amplitude.
class SpriteSpectrumScene : public Scene
{
public:
    static constexpr int kWidth = 320;
    static constexpr int kHeight = 200;
    static constexpr int kNumDisplayBands = 8;

    SpriteSpectrumScene();

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "sprite_spectrum"; }
    void cleanup(DoomEngine& engine) override;

private:
    std::vector<uint8_t> rgbaBuffer; // 320x200x4

    // Band amplitudes (smoothed)
    std::array<float, kNumDisplayBands> bandAmplitudes {};
    float backgroundBrightness = 0.0f;

    // Doom palette for indexed-to-RGB conversion
    uint8_t palette[768] = {};
    bool paletteLoaded = false;

    // Sprite IDs: use imp (SPR_TROO=0) for all bands
    static constexpr int kSpriteId = 0; // SPR_TROO

    void loadPalette(DoomEngine& engine);
    void clearBuffer();
    void drawPatchSprite(const uint8_t* patchData, int centerX, int baseY, float scale);
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
};
