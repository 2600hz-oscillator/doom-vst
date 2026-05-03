#pragma once

#include <array>
#include <atomic>
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

    // GUI-safe: request a one-shot preview at root pitch (MIDI 60, vel ~100).
    // The audio thread drains pending requests on its next processMidi call.
    // Lock-free atomic bitmask; idempotent within a block (multiple clicks
    // collapse to a single trigger per block).
    void requestPreview(int padIndex);

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

    // Bit i = preview requested for pad i. GUI thread sets via fetch_or;
    // audio thread atomic-exchanges to 0 once per block and triggers each
    // set bit. kNumPads ≤ 32 so a single uint32_t is sufficient.
    std::atomic<juce::uint32> pendingPreview { 0 };

    // Pre-sized scratch for one block of mono voice output. Sized in
    // prepare(); never reallocated on the audio thread.
    std::vector<float> voiceScratch {};
};
