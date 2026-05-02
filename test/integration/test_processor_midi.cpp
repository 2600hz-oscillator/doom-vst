#include "PluginProcessor.h"
#include <catch2/catch_test_macros.hpp>

namespace
{
    constexpr double kSampleRate = 44100.0;
    constexpr int    kBlockSize  = 512;

    void prepare(DoomVizProcessor& p)
    {
        p.setPlayConfigDetails(2, 2, kSampleRate, kBlockSize);
        p.prepareToPlay(kSampleRate, kBlockSize);
    }
}

TEST_CASE("DoomVizProcessor: processBlock routes NoteOn into SignalBus", "[processor][midi]")
{
    DoomVizProcessor p;
    prepare(p);

    juce::AudioBuffer<float> audio(2, kBlockSize);
    audio.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
    p.processBlock(audio, midi);

    REQUIRE(p.getSignalBus().getLastNote() == 60);
    REQUIRE(p.getSignalBus().getLastVelocity() == 100);
    REQUIRE(p.getSignalBus().isNoteHeld(60));
}

TEST_CASE("DoomVizProcessor: processBlock routes Program Change into SignalBus",
          "[processor][midi]")
{
    DoomVizProcessor p;
    prepare(p);

    juce::AudioBuffer<float> audio(2, kBlockSize);
    audio.clear();

    for (int pc = 0; pc < 3; ++pc)
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::programChange(1, pc), 0);
        p.processBlock(audio, midi);
        REQUIRE(p.getSignalBus().getProgramChange() == pc);
    }
}

TEST_CASE("DoomVizProcessor: processBlock routes CC and MidiClock", "[processor][midi]")
{
    DoomVizProcessor p;
    prepare(p);

    juce::AudioBuffer<float> audio(2, kBlockSize);
    audio.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 7, 64), 0);
    for (int i = 0; i < 24; ++i)
        midi.addEvent(juce::MidiMessage::midiClock(), i + 1);
    p.processBlock(audio, midi);

    REQUIRE(p.getSignalBus().getCC(7) == 64);
    REQUIRE(p.getSignalBus().getMidiClockCount() == 24);
}

TEST_CASE("DoomVizProcessor: empty MIDI buffer leaves SignalBus unchanged",
          "[processor][midi]")
{
    DoomVizProcessor p;
    prepare(p);

    juce::AudioBuffer<float> audio(2, kBlockSize);
    audio.clear();
    juce::MidiBuffer midi;
    p.processBlock(audio, midi);

    REQUIRE(p.getSignalBus().getLastNote() == -1);
    REQUIRE(p.getSignalBus().getProgramChange() == 0);
}
