#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class SignalBus;

// Parses MIDI messages from the audio thread and writes state to SignalBus.
class MidiHandler
{
public:
    explicit MidiHandler(SignalBus& bus);

    // Call from processBlock with the MIDI buffer for this block.
    void processMidiBuffer(const juce::MidiBuffer& buffer);

private:
    SignalBus& signalBus;
};
