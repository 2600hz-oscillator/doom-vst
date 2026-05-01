#pragma once

#include "Scene.h"
#include "patch/PatchSettings.h"
#include <vector>
#include <cstdint>
#include <array>

class AudioAnalyzer;
namespace patch { class PatchSettingsStore; }

// Scene D: Spectrum visualizer with acidwarp-style animated background.
// Background pattern parameters are driven by individual frequency bands.
// Per-band sprites: cacodemons (bass), imps (mids), zombiemen + shotgun guys (highs).
class Spectrum2Scene : public Scene
{
public:
    static constexpr int kWidth = 320;
    static constexpr int kHeight = 200;
    static constexpr int kNumDisplayBands = patch::kSpectrumNumBands;

    Spectrum2Scene(const AudioAnalyzer& analyzer,
                   const patch::PatchSettingsStore& patchStore);

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "spectrum2"; }
    void cleanup(DoomEngine& engine) override;

private:
    const AudioAnalyzer& analyzer;
    const patch::PatchSettingsStore& patchStore;
    std::vector<uint8_t> rgbaBuffer;

    // Per-band amplitudes (post-envelope, post-normalization, in [0,1])
    std::array<float, kNumDisplayBands> bandAmplitudes {};
    // Per-band slider gain pulled from settings each Apply (0..1).
    std::array<float, kNumDisplayBands> bandGains01 {};
    // Per-band sprite ID pulled from settings each frame.
    std::array<int,   kNumDisplayBands> bandSpriteIds {};
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

    void initSinTable();
    void loadPalette(DoomEngine& engine);
    void renderAcidwarpBackground();
    void drawPatchSprite(const uint8_t* patchData, int centerX, int baseY, float scale);
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    inline float fastSin(float x) const;
};
