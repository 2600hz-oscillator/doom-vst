#pragma once

#include "Scene.h"
#include <vector>
#include <cstdint>
#include <array>

class AudioAnalyzer;

// Scene D: Spectrum visualizer with acidwarp-style animated background.
// Background pattern parameters are driven by individual frequency bands.
// Per-band sprites: cacodemons (bass), imps (mids), zombiemen + shotgun guys (highs).
class Spectrum2Scene : public Scene
{
public:
    static constexpr int kWidth = 320;
    static constexpr int kHeight = 200;
    static constexpr int kNumDisplayBands = 8;

    Spectrum2Scene(const AudioAnalyzer& analyzer);

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "spectrum2"; }
    void cleanup(DoomEngine& engine) override;

private:
    const AudioAnalyzer& analyzer;
    std::vector<uint8_t> rgbaBuffer;

    std::array<float, kNumDisplayBands> bandAmplitudes {};
    float overallRMS = 0.0f;
    bool onsetTrigger = false;

    // Acidwarp animation parameters (driven by audio bands)
    float time = 0.0f;
    float paletteShift = 0.0f;

    // Doom palette
    uint8_t palette[768] = {};
    bool paletteLoaded = false;

    // Sine LUT for fast pattern generation
    static constexpr int kSinTableSize = 1024;
    std::array<float, kSinTableSize> sinTable {};

    // Sprite per band: cacodemon, imp, zombieman, shotgun guy
    static constexpr int kSpritePlay = 28;  // Doom marine (player)
    static constexpr int kSpriteTroop = 0;  // Imp
    static constexpr int kSpritePoss = 29;  // Zombieman
    static constexpr int kSpriteSpos = 30;  // Shotgun guy

    static constexpr int kBandSprites[kNumDisplayBands] = {
        kSpritePlay,  // 0: sub-bass — Doom marine
        kSpritePlay,  // 1: bass — Doom marine
        kSpriteTroop, // 2: low-mid — imp
        kSpriteTroop, // 3: mid — imp
        kSpriteTroop, // 4: upper-mid — imp
        kSpriteTroop, // 5: presence — imp
        kSpritePoss,  // 6: brilliance — zombieman
        kSpriteSpos,  // 7: air — shotgun guy
    };

    void initSinTable();
    void loadPalette(DoomEngine& engine);
    void renderAcidwarpBackground();
    void drawPatchSprite(const uint8_t* patchData, int centerX, int baseY, float scale);
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    inline float fastSin(float x) const;
};
