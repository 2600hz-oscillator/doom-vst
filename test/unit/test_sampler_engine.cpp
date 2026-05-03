#include "audio/SamplerEngine.h"
#include "patch/VisualizerState.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
    patch::SamplerConfig oneTonePad(int padIdx, std::vector<float> data,
                                     int start = 0, int end = 0)
    {
        auto cfg = patch::SamplerConfig::makeDefault();
        cfg.pads[padIdx].name             = "test.wav";
        cfg.pads[padIdx].sourceSampleRate = 48000.0;
        cfg.pads[padIdx].startSample      = start;
        cfg.pads[padIdx].endSample        = end > 0 ? end : static_cast<int>(data.size());
        cfg.pads[padIdx].sampleData       = std::move(data);
        return cfg;
    }
}

TEST_CASE("SamplerEngine: noteOn allocates a voice on a loaded pad",
          "[sampler][engine]")
{
    patch::VisualizerState state;
    state.setSampler(oneTonePad(0, std::vector<float>(64, 0.5f)));

    SamplerEngine eng(state);
    eng.prepare(48000.0, 512);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(/*channel*/ 1, 60, (juce::uint8) 100), 0);
    eng.processMidi(midi);

    REQUIRE(eng.activeVoiceCount(0) == 1);
}

TEST_CASE("SamplerEngine: noteOn on an empty pad does not allocate",
          "[sampler][engine]")
{
    patch::VisualizerState state;  // all pads empty
    SamplerEngine eng(state);
    eng.prepare(48000.0, 512);

    eng.noteOn(0, 60, 1.0f);
    REQUIRE(eng.activeVoiceCount(0) == 0);
}

TEST_CASE("SamplerEngine: 6th retrigger steals the oldest voice (5-voice pool)",
          "[sampler][engine]")
{
    patch::VisualizerState state;
    state.setSampler(oneTonePad(0, std::vector<float>(48000, 0.5f)));

    SamplerEngine eng(state);
    eng.prepare(48000.0, 512);

    juce::AudioBuffer<float> buf(2, 512);
    buf.clear();

    auto fireAndAge = [&]()
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
        eng.processMidi(midi);
        eng.render(buf, 512);
    };

    for (int i = 0; i < 5; ++i)
        fireAndAge();
    REQUIRE(eng.activeVoiceCount(0) == 5);

    fireAndAge();
    REQUIRE(eng.activeVoiceCount(0) == 5); // still 5, but the oldest got stolen
}

TEST_CASE("SamplerEngine: MIDI channels 10..16 do not trigger sampler voices",
          "[sampler][engine]")
{
    patch::VisualizerState state;
    state.setSampler(oneTonePad(0, std::vector<float>(64, 0.5f)));

    SamplerEngine eng(state);
    eng.prepare(48000.0, 512);

    juce::MidiBuffer midi;
    for (int ch = 10; ch <= 16; ++ch)
        midi.addEvent(juce::MidiMessage::noteOn(ch, 60, (juce::uint8) 100), 0);
    eng.processMidi(midi);

    for (int p = 0; p < patch::kNumPads; ++p)
        REQUIRE(eng.activeVoiceCount(p) == 0);
}

TEST_CASE("SamplerEngine: render produces non-silent output after a NoteOn",
          "[sampler][engine]")
{
    patch::VisualizerState state;
    state.setSampler(oneTonePad(0, std::vector<float>(1024, 0.25f)));

    SamplerEngine eng(state);
    eng.prepare(48000.0, 256);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 127), 0);
    eng.processMidi(midi);

    juce::AudioBuffer<float> buf(2, 256);
    buf.clear();
    eng.render(buf, 256);

    bool nonSilent = false;
    for (int i = 0; i < 256 && ! nonSilent; ++i)
        if (buf.getReadPointer(0)[i] != 0.0f) nonSilent = true;
    REQUIRE(nonSilent);
}
