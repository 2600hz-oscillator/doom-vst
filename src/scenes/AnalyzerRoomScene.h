#pragma once

#include "Scene.h"
#include <vector>
#include <cstdint>
#include <array>

class AudioAnalyzer;
class SignalBus;

// Scene C: Inside the Doom engine, holding the BFG9000.
// Camera rotates in place at BPM rate. FFT spectrum is injected
// onto a wall texture so it appears on the walls around the player.
class AnalyzerRoomScene : public Scene
{
public:
    static constexpr int kNumBars = 8;

    AnalyzerRoomScene(const AudioAnalyzer& analyzer, const SignalBus& bus);

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "analyzer_room"; }
    void cleanup(DoomEngine& engine) override;

private:
    const AudioAnalyzer& analyzer;
    const SignalBus& signalBus;

    // Camera
    int32_t camX = 0, camY = 0, camZ = 0;
    uint32_t camAngle = 0;

    // Rotation
    float bpm = 120.0f;
    int lastClockCount = 0;

    // Wall texture buffer (column-major, allocated after getting texture dims)
    std::vector<uint8_t> texPixels;
    int texWidth = 0, texHeight = 0;
    bool texInitialized = false;

    // The wall texture we replace
    static constexpr const char* kTargetTexture = "STARTAN3";

    // Bar colors as Doom palette indices (approximate)
    static constexpr uint8_t kBarPalette[kNumBars] = {
        176, // red
        208, // orange
        160, // yellow
        112, // green
        192, // cyan
        200, // blue
        248, // purple
        252, // magenta/pink
    };

    void renderFFTToTexture();
};
