// Integration test: real DoomVizProcessor + a loaded sampler pad +
// MIDI NoteOn → captured audio output buffer is non-silent.
#define DOOMVIZ_TEST_BUILD 1
#include "PluginProcessor.h"
#include "patch/VisualizerState.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
    patch::SamplerConfig padWithRamp(int padIdx, int n, float peak = 0.5f)
    {
        std::vector<float> data(n);
        for (int i = 0; i < n; ++i)
            data[i] = peak * (static_cast<float>(i) / static_cast<float>(n));

        auto cfg = patch::SamplerConfig::makeDefault();
        cfg.pads[padIdx].name             = "ramp.wav";
        cfg.pads[padIdx].sourceSampleRate = 48000.0;
        cfg.pads[padIdx].startSample      = 0;
        cfg.pads[padIdx].endSample        = n;
        cfg.pads[padIdx].sampleData       = std::move(data);
        return cfg;
    }
}

TEST_CASE("DoomVizProcessor: NoteOn on ch 1 with a loaded pad mixes audio output",
          "[sampler][integration]")
{
    DoomVizProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    auto& state = proc.getVisualizerState();
    state.setSampler(padWithRamp(0, 4096));

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);

    juce::AudioBuffer<float> buf(2, 512);
    buf.clear(); // simulate silent input — anything non-zero in the output
                 // came from the sampler mix.
    proc.processBlock(buf, midi);

    bool nonSilent = false;
    for (int ch = 0; ch < buf.getNumChannels() && ! nonSilent; ++ch)
        for (int i = 0; i < buf.getNumSamples() && ! nonSilent; ++i)
            if (buf.getReadPointer(ch)[i] != 0.0f) nonSilent = true;
    REQUIRE(nonSilent);
}

TEST_CASE("DoomVizProcessor: silent MIDI + silent input yields silent output",
          "[sampler][integration]")
{
    DoomVizProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    juce::MidiBuffer empty;
    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    proc.processBlock(buf, empty);

    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            REQUIRE(buf.getReadPointer(ch)[i] == 0.0f);
}

TEST_CASE("DoomVizProcessor: setPadMarkers honored by next NoteOn",
          "[sampler][integration]")
{
    DoomVizProcessor proc;
    proc.prepareToPlay(48000.0, 64);

    auto& state = proc.getVisualizerState();

    // Pad with: silence in the first half, full-amplitude tone in the second.
    auto cfg = patch::SamplerConfig::makeDefault();
    cfg.pads[0].name             = "halftone.wav";
    cfg.pads[0].sourceSampleRate = 48000.0;
    cfg.pads[0].startSample      = 0;
    cfg.pads[0].endSample        = 256;

    std::vector<float> data(256, 0.0f);
    for (int i = 128; i < 256; ++i) data[i] = 0.9f;
    cfg.pads[0].sampleData = std::move(data);
    state.setSampler(cfg);

    // First trigger across the whole sample — the back half is non-silent.
    {
        juce::MidiBuffer m;
        m.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 127), 0);
        juce::AudioBuffer<float> buf(2, 64);
        buf.clear();
        proc.processBlock(buf, m);
        // First 64 samples → silence (front half of source).
        for (int i = 0; i < 64; ++i)
            REQUIRE(buf.getReadPointer(0)[i] == 0.0f);
    }

    // Trim playback to start at 128 → the very first output sample is loud.
    state.setPadMarkers(0, 128, 256);

    {
        juce::MidiBuffer m;
        m.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 127), 0);
        juce::AudioBuffer<float> buf(2, 64);
        buf.clear();
        proc.processBlock(buf, m);
        // The trimmed range is the loud half — first sample should be > 0.5.
        REQUIRE(buf.getReadPointer(0)[0] > 0.5f);
    }
}

TEST_CASE("DoomVizProcessor: ch 10 NoteOn does not trigger any sampler voice",
          "[sampler][integration]")
{
    DoomVizProcessor proc;
    proc.prepareToPlay(48000.0, 512);

    auto& state = proc.getVisualizerState();
    state.setSampler(padWithRamp(0, 4096));

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(10, 60, (juce::uint8) 127), 0);

    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();
    proc.processBlock(buf, midi);

    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        for (int i = 0; i < buf.getNumSamples(); ++i)
            REQUIRE(buf.getReadPointer(ch)[i] == 0.0f);
}
