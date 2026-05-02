#pragma once

#include "Scene.h"
#include "patch/VisualizerState.h"
#include <vector>
#include <cstdint>
#include <array>

class AudioAnalyzer;
class SignalBus;

// Scene C: Inside the Doom engine holding BFG9000.
// Player wanders E1M1 with collision avoidance. The FFT spectrum, sliced
// into the user's 8 global bands, is injected onto a wall texture so the
// bars appear on walls around the player.
class AnalyzerRoomScene : public Scene
{
public:
    static constexpr int kNumBars = patch::kNumBands;

    AnalyzerRoomScene(const AudioAnalyzer& analyzer,
                      const SignalBus& bus,
                      const patch::VisualizerState& vizState);

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "analyzer_room"; }
    void cleanup(DoomEngine& engine) override;

private:
    const AudioAnalyzer& analyzer;
    const SignalBus& signalBus;
    const patch::VisualizerState& vizState;

    // Per-band envelopes fed by FFT bins inside the user's lowHz/highHz
    // ranges. Same envelope follower Spectrum2/KillRoom use, so all three
    // scenes interpret the audio consistently.
    std::array<float, kNumBars> bandAmplitudes {};

    // Camera / navigation
    int32_t camX = 0, camY = 0, camZ = 0;
    uint32_t camAngle = 0;
    float walkSpeed = 400.0f;
    float turnTimer = 0.0f;
    float turnDirection = 1.0f;
    float blockedTimer = 0.0f;

    // Wall texture buffer
    std::vector<uint8_t> texPixels;
    int texWidth = 0, texHeight = 0;
    bool texInitialized = false;

    static constexpr const char* kTargetTexture = "STARTAN3";

    static constexpr uint8_t kBarPalette[kNumBars] = {
        176, 208, 160, 112, 192, 200, 248, 252
    };

    void updateBands(float deltaTime);
    void updateNavigation(DoomEngine& engine, float deltaTime);
    void renderFFTToTexture();
};
