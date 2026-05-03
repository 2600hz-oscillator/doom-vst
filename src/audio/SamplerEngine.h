#pragma once

#include <array>
#include <juce_audio_basics/juce_audio_basics.h>
#include "VarispeedEngine.h"
#include "patch/VisualizerState.h"

// 9-pad sample player. Audio thread only. Each pad has 5 voices with
// steal-oldest behaviour, so the 9 MIDI channels can each polyphonically
// retrigger without unbounded voice growth.
class SamplerEngine
{
public:
    explicit SamplerEngine(const patch::VisualizerState& state);

    void prepare(double hostSampleRate, int samplesPerBlock);

    // Audio thread. Refreshes the engine's snapshot of SamplerConfig and
    // dispatches NoteOn on channels 1..9 to noteOn(). Channels 10..16 are
    // ignored (still consumed by MidiHandler for visualizer state).
    void processMidi(const juce::MidiBuffer& midi);

    // Audio thread. Mixes active voices on top of `buffer`. Caller is
    // responsible for any input passthrough already in the buffer.
    void render(juce::AudioBuffer<float>& buffer, int numSamples);

    // Visible for testing — equivalent to receiving a MIDI NoteOn.
    void noteOn(int padIndex, int midiNote, float velocity01);

    // Visible for testing — number of currently-active voices on a pad.
    int activeVoiceCount(int padIndex) const;

private:
    static constexpr int kVoicesPerPad = 5;

    struct Voice
    {
        bool  active       { false };
        int   midiNote     { 60 };
        float gain         { 0.0f };
        int   ageInBlocks  { 0 };
        int   sampleStart  { 0 };
        int   sampleEnd    { 0 };
        VarispeedEngine engine {};
    };

    using PadVoices = std::array<Voice, kVoicesPerPad>;

    const patch::VisualizerState& vizState;
    double sampleRate { 44100.0 };
    patch::SamplerConfig liveConfig { patch::SamplerConfig::makeDefault() };
    std::array<PadVoices, patch::kNumPads> voices {};

    // Pre-sized scratch for one block of mono voice output. Sized in
    // prepare(); never reallocated on the audio thread.
    std::vector<float> voiceScratch {};
};
