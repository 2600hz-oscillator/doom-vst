#include "PluginProcessor.h"
#include "PluginEditor.h"

DoomVizProcessor::DoomVizProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

DoomVizProcessor::~DoomVizProcessor() = default;

void DoomVizProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
}

void DoomVizProcessor::releaseResources()
{
}

void DoomVizProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midiMessages*/)
{
    // Pass audio through unchanged.
    // In Phase 4 we'll push samples to the SignalBus and parse MIDI here.
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels beyond the input count
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* DoomVizProcessor::createEditor()
{
    return new DoomVizEditor(*this);
}

void DoomVizProcessor::getStateInformation(juce::MemoryBlock& /*destData*/)
{
}

void DoomVizProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DoomVizProcessor();
}
