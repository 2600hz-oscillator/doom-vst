#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>

// Lock-free communication from audio thread to render thread.
// Audio thread writes; render thread reads.
class SignalBus
{
public:
    SignalBus(int ringBufferSize = 8192);

    // --- Audio thread writes ---

    // Push mono-downmixed audio samples into the ring buffer.
    void pushAudioSamples(const float* data, int numSamples);

    // Update MIDI state (called from audio thread).
    void setNoteOn(int note, int velocity);
    void setNoteOff(int note);
    void setCC(int ccNumber, int value);
    void setProgramChange(int program);
    void incrementMidiClock();

    // --- Render thread reads ---

    // Pull available audio samples. Returns number of samples actually read.
    int pullAudioSamples(float* dest, int maxSamples);

    // Read current MIDI state.
    int getLastNote() const { return lastNote.load(std::memory_order_relaxed); }
    int getLastVelocity() const { return lastVelocity.load(std::memory_order_relaxed); }
    int getProgramChange() const { return lastProgramChange.load(std::memory_order_relaxed); }
    int getCC(int ccNumber) const;
    int getMidiClockCount() const { return midiClockCount.load(std::memory_order_relaxed); }

    // Note-on state per note (for polyphonic tracking)
    bool isNoteHeld(int note) const;

private:
    // Ring buffer for audio samples
    juce::AbstractFifo fifo;
    std::vector<float> ringBuffer;

    // Atomic MIDI state
    std::atomic<int> lastNote { -1 };
    std::atomic<int> lastVelocity { 0 };
    std::atomic<int> lastProgramChange { 0 };
    std::atomic<int> midiClockCount { 0 };
    std::array<std::atomic<int>, 128> ccValues;
    std::array<std::atomic<bool>, 128> noteState;
};
