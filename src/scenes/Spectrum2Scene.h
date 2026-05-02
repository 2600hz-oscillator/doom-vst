#pragma once

#include "Scene.h"
#include "patch/VisualizerState.h"
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
    static constexpr int kNumDisplayBands = patch::kNumBands;

    Spectrum2Scene(const AudioAnalyzer& analyzer,
                   const patch::VisualizerState& vizState);

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "spectrum2"; }
    void cleanup(DoomEngine& engine) override;

private:
    const AudioAnalyzer& analyzer;
    const patch::VisualizerState& vizState;
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

    // --- DOOMTEX state ---
    // Index into kDoomtexNames (advances when band 3 envelope crosses 50%).
    int doomtexIndex = 0;
    bool doomtexAboveThreshold = false;
    // Cached pixel data for the current texture (column-major indexed bytes).
    std::vector<uint8_t> doomtexPixels;
    int doomtexW = 0, doomtexH = 0;
    int doomtexCachedIndex = -1;
    void cacheDoomtex(int idx);

    // --- STARFIELD state ---
    struct Star { float x, y, z; };
    static constexpr int kNumStars = 160;
    std::array<Star, kNumStars> stars {};
    bool starsInitialized = false;
    void initStars();

    // --- WINAMP peak hold state ---
    std::array<float, kNumDisplayBands> winampPeak {};

    void initSinTable();
    void loadPalette(DoomEngine& engine);

    void renderBackground(patch::BackgroundVibe vibe);
    void renderAcidwarpBackground();
    void renderVaporwave();
    void renderPunkrock();
    void renderDoomtex();
    void renderWinamp();
    void renderStarfield(float deltaTime);
    void renderCrtglitch();

    void drawPatchSprite(const uint8_t* patchData, int centerX, int baseY, float scale);
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
    void fillRect(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
    void clearBuffer(uint8_t r, uint8_t g, uint8_t b);
    inline float fastSin(float x) const;

    // Last deltaTime captured in update so render() can use it (e.g., starfield).
    float lastDelta = 1.0f / 60.0f;

    // Active background per latest snapshot.
    patch::BackgroundVibe currentVibe { patch::BackgroundVibe::AcidwarpExe };
};
