#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "audio/SignalBus.h"
#include "audio/MidiHandler.h"
#include "audio/SamplerEngine.h"
#include "patch/VisualizerState.h"
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

    // The scene the renderer is currently displaying. DoomViewport sets this
    // after every switch (whether driven by sceneOverride or by MIDI Program
    // Change). The editor polls it and pushes scene-aware UI changes (e.g.
    // showing the right Patch panel). This is the single source of truth so
    // MIDI-triggered switches stay in sync with the GUI.
    std::atomic<int> currentSceneIndex { 0 };

    // Persistent shared state for the visualizer (global band config +
    // per-scene config). GUI thread writes via the control panel's Apply
    // button; render thread reads via snapshot copy under SpinLock.
    patch::VisualizerState& getVisualizerState() { return visualizerState; }
    const patch::VisualizerState& getVisualizerState() const { return visualizerState; }

    // Used by the SFX tab's TRIG buttons to fire a one-shot preview at
    // root pitch. requestPreview is the only audio-thread-safe entry
    // point on SamplerEngine for callers outside processBlock.
    SamplerEngine& getSamplerEngine() { return samplerEngine; }

private:
    SignalBus signalBus;
    MidiHandler midiHandler;
    patch::VisualizerState visualizerState;
    SamplerEngine samplerEngine { visualizerState };
    std::vector<float> monoBuffer;
    double currentSampleRate = 44100.0;

    // One-shot guard for the bundled default-kit auto-load. Set on the
    // first prepareToPlay; never reloads thereafter. A user who clears
    // all pads and saves keeps that state on subsequent reloads.
    bool defaultKitAttempted = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoomVizProcessor)
};
