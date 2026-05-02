#include "PluginProcessor.h"
#ifndef DOOMVIZ_TEST_BUILD
#include "PluginEditor.h"
#endif

DoomVizProcessor::DoomVizProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      midiHandler(signalBus)
{
}

DoomVizProcessor::~DoomVizProcessor() = default;

void DoomVizProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    monoBuffer.resize(static_cast<size_t>(samplesPerBlock), 0.0f);
}

void DoomVizProcessor::releaseResources()
{
}

void DoomVizProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // Clear any output channels beyond the input count
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Mono downmix into ring buffer for the analyzer
    if (totalNumInputChannels > 0 && numSamples > 0)
    {
        if (static_cast<int>(monoBuffer.size()) < numSamples)
            monoBuffer.resize(static_cast<size_t>(numSamples));

        // Start with first channel
        std::copy(buffer.getReadPointer(0),
                  buffer.getReadPointer(0) + numSamples,
                  monoBuffer.data());

        // Mix in remaining channels
        for (int ch = 1; ch < totalNumInputChannels; ++ch)
        {
            const float* src = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                monoBuffer[static_cast<size_t>(i)] += src[i];
        }

        // Normalize
        float scale = 1.0f / static_cast<float>(totalNumInputChannels);
        for (int i = 0; i < numSamples; ++i)
            monoBuffer[static_cast<size_t>(i)] *= scale;

        signalBus.pushAudioSamples(monoBuffer.data(), numSamples);
    }

    // Parse MIDI events
    midiHandler.processMidiBuffer(midiMessages);

    // Audio passes through unchanged
}

#ifndef DOOMVIZ_TEST_BUILD
juce::AudioProcessorEditor* DoomVizProcessor::createEditor()
{
    return new DoomVizEditor(*this);
}
#endif

void DoomVizProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::String xml = visualizerState.toXmlString();
    destData.replaceAll(xml.toRawUTF8(), xml.getNumBytesAsUTF8());
}

void DoomVizProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0) return;
    juce::String xml = juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    visualizerState.fromXmlString(xml);
}

#ifndef DOOMVIZ_TEST_BUILD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DoomVizProcessor();
}
#endif
