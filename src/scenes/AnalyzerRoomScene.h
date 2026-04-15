#pragma once

#include "Scene.h"
#include <vector>
#include <cstdint>
#include <array>

class AudioAnalyzer;

// Scene C: Auto-navigating E1M1 with an FFT spectrum rendered onto a floor texture.
// The player auto-walks while the analyzer output is drawn to a Doom flat.
class AnalyzerRoomScene : public Scene
{
public:
    AnalyzerRoomScene(const AudioAnalyzer& analyzer);

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "analyzer_room"; }
    void cleanup(DoomEngine& engine) override;

private:
    const AudioAnalyzer& analyzer;

    // Camera auto-navigation
    int32_t camX = 0, camY = 0, camZ = 0;
    uint32_t camAngle = 0;
    float turnTimer = 0.0f;
    float turnDirection = 1.0f;
    float blockedTimer = 0.0f;

    // FFT texture data (64x64 indexed pixels for a Doom flat)
    std::array<uint8_t, 64 * 64> flatPixels {};

    void updateCamera(DoomEngine& engine, const ParameterMap& params, float deltaTime);
    void renderFFTToFlat();
};
