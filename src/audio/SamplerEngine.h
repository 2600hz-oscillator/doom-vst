#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace patch { class VisualizerState; }

// 9-pad sample player. Audio thread only. Phase-2 stub: no voices, no
// playback — just the call-site so processBlock has a stable seam to mix
// into. Phase-3 lands the VarispeedEngine + voice pool; Phase-4 wires
// MIDI triggers.
class SamplerEngine
{
public:
    explicit SamplerEngine(const patch::VisualizerState& state);

    void prepare(double hostSampleRate);

    // Mix active voices into `buffer`. Caller's audio is already in the
    // buffer (passthrough); we add on top. No clear, no replace.
    void render(juce::AudioBuffer<float>& buffer, int numSamples);

private:
    const patch::VisualizerState& vizState;
    double sampleRate { 44100.0 };
};
