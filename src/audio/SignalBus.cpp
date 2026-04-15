#include "SignalBus.h"

SignalBus::SignalBus(int ringBufferSize)
    : fifo(ringBufferSize), ringBuffer(static_cast<size_t>(ringBufferSize), 0.0f)
{
    for (auto& cc : ccValues)
        cc.store(0, std::memory_order_relaxed);
    for (auto& ns : noteState)
        ns.store(false, std::memory_order_relaxed);
}

void SignalBus::pushAudioSamples(const float* data, int numSamples)
{
    const auto scope = fifo.write(numSamples);

    if (scope.blockSize1 > 0)
        std::copy(data, data + scope.blockSize1, ringBuffer.data() + scope.startIndex1);
    if (scope.blockSize2 > 0)
        std::copy(data + scope.blockSize1, data + scope.blockSize1 + scope.blockSize2,
                  ringBuffer.data() + scope.startIndex2);
}

int SignalBus::pullAudioSamples(float* dest, int maxSamples)
{
    const auto ready = fifo.getNumReady();
    const auto toRead = std::min(ready, maxSamples);
    if (toRead == 0)
        return 0;

    const auto scope = fifo.read(toRead);

    if (scope.blockSize1 > 0)
        std::copy(ringBuffer.data() + scope.startIndex1,
                  ringBuffer.data() + scope.startIndex1 + scope.blockSize1,
                  dest);
    if (scope.blockSize2 > 0)
        std::copy(ringBuffer.data() + scope.startIndex2,
                  ringBuffer.data() + scope.startIndex2 + scope.blockSize2,
                  dest + scope.blockSize1);

    return toRead;
}

void SignalBus::setNoteOn(int note, int velocity)
{
    if (note >= 0 && note < 128)
    {
        lastNote.store(note, std::memory_order_relaxed);
        lastVelocity.store(velocity, std::memory_order_relaxed);
        noteState[static_cast<size_t>(note)].store(true, std::memory_order_relaxed);
    }
}

void SignalBus::setNoteOff(int note)
{
    if (note >= 0 && note < 128)
        noteState[static_cast<size_t>(note)].store(false, std::memory_order_relaxed);
}

void SignalBus::setCC(int ccNumber, int value)
{
    if (ccNumber >= 0 && ccNumber < 128)
        ccValues[static_cast<size_t>(ccNumber)].store(value, std::memory_order_relaxed);
}

void SignalBus::setProgramChange(int program)
{
    lastProgramChange.store(program, std::memory_order_relaxed);
}

void SignalBus::incrementMidiClock()
{
    midiClockCount.fetch_add(1, std::memory_order_relaxed);
}

int SignalBus::getCC(int ccNumber) const
{
    if (ccNumber >= 0 && ccNumber < 128)
        return ccValues[static_cast<size_t>(ccNumber)].load(std::memory_order_relaxed);
    return 0;
}

bool SignalBus::isNoteHeld(int note) const
{
    if (note >= 0 && note < 128)
        return noteState[static_cast<size_t>(note)].load(std::memory_order_relaxed);
    return false;
}
