#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "audio/SignalBus.h"
#include "audio/MidiHandler.h"
#include <memory>

class DoomVizProcessor : public juce::AudioProcessor
{
public:
    DoomVizProcessor();
    ~DoomVizProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "DoomViz"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Shared signal bus — render thread reads from this
    SignalBus& getSignalBus() { return signalBus; }
    double getCurrentSampleRate() const { return currentSampleRate; }

    // Scene override from control window (-1 = no override, use MIDI PC)
    std::atomic<int> sceneOverride { -1 };

private:
    SignalBus signalBus;
    MidiHandler midiHandler;
    std::vector<float> monoBuffer;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoomVizProcessor)
};
