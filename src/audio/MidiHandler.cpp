#include "MidiHandler.h"
#include "SignalBus.h"

MidiHandler::MidiHandler(SignalBus& bus)
    : signalBus(bus)
{
}

void MidiHandler::processMidiBuffer(const juce::MidiBuffer& buffer)
{
    for (const auto metadata : buffer)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
            signalBus.setNoteOn(msg.getNoteNumber(), msg.getVelocity());
        else if (msg.isNoteOff())
            signalBus.setNoteOff(msg.getNoteNumber());
        else if (msg.isController())
            signalBus.setCC(msg.getControllerNumber(), msg.getControllerValue());
        else if (msg.isProgramChange())
            signalBus.setProgramChange(msg.getProgramChangeNumber());
        else if (msg.isMidiClock())
            signalBus.incrementMidiClock();
    }
}
