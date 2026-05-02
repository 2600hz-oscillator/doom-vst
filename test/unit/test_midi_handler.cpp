#include "audio/MidiHandler.h"
#include "audio/SignalBus.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("MidiHandler: NoteOn/NoteOff routes to SignalBus", "[midi_handler]")
{
    SignalBus bus;
    MidiHandler handler(bus);

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 0);
    handler.processMidiBuffer(buf);
    REQUIRE(bus.getLastNote() == 60);
    REQUIRE(bus.getLastVelocity() == 100);
    REQUIRE(bus.isNoteHeld(60));

    juce::MidiBuffer off;
    off.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    handler.processMidiBuffer(off);
    REQUIRE_FALSE(bus.isNoteHeld(60));
}

TEST_CASE("MidiHandler: CC, ProgramChange, MidiClock route to SignalBus", "[midi_handler]")
{
    SignalBus bus;
    MidiHandler handler(bus);

    juce::MidiBuffer buf;
    buf.addEvent(juce::MidiMessage::controllerEvent(1, 7, 64), 0);
    buf.addEvent(juce::MidiMessage::programChange(1, 2), 1);
    buf.addEvent(juce::MidiMessage::midiClock(), 2);
    buf.addEvent(juce::MidiMessage::midiClock(), 3);
    handler.processMidiBuffer(buf);

    REQUIRE(bus.getCC(7) == 64);
    REQUIRE(bus.getProgramChange() == 2);
    REQUIRE(bus.getMidiClockCount() == 2);
}

TEST_CASE("MidiHandler: empty buffer leaves SignalBus untouched", "[midi_handler]")
{
    SignalBus bus;
    MidiHandler handler(bus);
    juce::MidiBuffer empty;
    handler.processMidiBuffer(empty);
    REQUIRE(bus.getLastNote() == -1);
    REQUIRE(bus.getProgramChange() == 0);
}
