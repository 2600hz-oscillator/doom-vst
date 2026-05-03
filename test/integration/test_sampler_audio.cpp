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
