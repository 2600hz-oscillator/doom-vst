#include "PluginProcessor.h"
#include "patch/SampleSetIO.h"
#ifndef DOOMVIZ_TEST_BUILD
#include "PluginEditor.h"
#endif

namespace
{
    // Mirrors DoomViewport::findWadPath — locate the bundled
    // Contents/Resources/sets/default.dvset whether we're running
    // inside a VST3 bundle, alongside it, or in the dev-tree CWD.
    juce::File findBundledDefaultSet()
    {
        auto thisModule = juce::File::getSpecialLocation(
            juce::File::currentExecutableFile);
        auto bundleRoot = thisModule.getParentDirectory()
                                    .getParentDirectory()
                                    .getParentDirectory();

        const juce::String rel = "Contents/Resources/sets/default.dvset";
        auto bundled = bundleRoot.getChildFile(rel);
        if (bundled.existsAsFile()) return bundled;

        auto nextToBundle = bundleRoot.getParentDirectory()
                                       .getChildFile("default.dvset");
        if (nextToBundle.existsAsFile()) return nextToBundle;

        auto devPath = juce::File::getCurrentWorkingDirectory()
                           .getChildFile("resources/sets/default.dvset");
        if (devPath.existsAsFile()) return devPath;

        return {};
    }
}

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
    samplerEngine.prepare(sampleRate, samplesPerBlock);

    // Auto-load the bundled default kit on the *first* prepareToPlay so a
    // fresh DAW instance comes up ready to play. The host calls
    // setStateInformation before prepareToPlay when restoring a project
    // — if any pad has sample data at this point, the user has saved
    // state and we leave it alone. The flag ensures we only attempt
    // once: a user who deliberately empties all pads + saves keeps that
    // state on subsequent loads.
    if (! defaultKitAttempted)
    {
        defaultKitAttempted = true;
        const auto live = visualizerState.getSampler();
        const bool anyLoaded = std::any_of(live.pads.begin(), live.pads.end(),
            [](const auto& p) { return ! p.sampleData.empty(); });
        if (! anyLoaded)
        {
            auto setFile = findBundledDefaultSet();
            if (setFile != juce::File{})
            {
                if (auto loaded = patch::SampleSetIO::loadFromFile(setFile, sampleRate))
                    visualizerState.setSampler(*loaded);
            }
        }
    }
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

    // Sampler MIDI: ch 1-9 NoteOn → pad triggers. Refreshes its
    // SamplerConfig snapshot before dispatching, so newly-loaded pads
    // are audible from the next block onward.
    samplerEngine.processMidi(midiMessages);

    // Mix sampler voices on top of the input passthrough.
    samplerEngine.render(buffer, numSamples);
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
