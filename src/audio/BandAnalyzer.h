#pragma once

#include "patch/VisualizerState.h"
#include <array>

class AudioAnalyzer;

// Per-band envelope follower fed by the AudioAnalyzer's raw FFT magnitude
// spectrum, sliced into the user's 8 global bands (Hz ranges from
// GlobalConfig). Spectrum2, KillRoom, and AnalyzerRoom all run the same
// "FFT bins → 8 band amps + peak-normalize + one-pole envelope" pipeline,
// so it lives here as a small reusable component.
//
// Usage:
//   class MyScene { BandAnalyzer bands; ... };
//   void MyScene::init(...)   { bands.reset(); }
//   void MyScene::update(...) { bands.update(analyzer, globalCfg, dt); }
//   ... bands[i] ...   // smoothed amplitude in [0, 1]
//
// All work happens on the render thread; not safe to call concurrently
// from multiple threads.
class BandAnalyzer
{
public:
    static constexpr int kNumBands = patch::kNumBands;

    BandAnalyzer() = default;

    // Compute one frame's per-band amplitudes from the analyzer's current
    // FFT spectrum, peak-normalize across the 8 bands, then advance the
    // envelope follower by deltaTime.
    void update(const AudioAnalyzer& analyzer,
                const patch::GlobalConfig& global,
                float deltaTime);

    // Zero the envelope state. Scenes call this from init() to drop
    // animation history without touching configuration.
    void reset() { envelope.fill(0.0f); }

    // Smoothed amplitude in [0, 1] for band i.
    float operator[](size_t i) const { return envelope[i]; }

    // Pointer to the underlying 8-element array — handy for code paths
    // that need contiguous storage (e.g. memcpy into a UI buffer).
    const std::array<float, kNumBands>& amplitudes() const { return envelope; }

private:
    std::array<float, kNumBands> envelope {};

    // Envelope follower time constants (seconds). Match the values the
    // three scenes used pre-extraction so visuals stay byte-identical.
    static constexpr float kAttack  = 0.01f;
    static constexpr float kRelease = 0.15f;
};
