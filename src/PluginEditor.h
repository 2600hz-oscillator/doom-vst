#pragma once

#include "PluginProcessor.h"
#include "DoomViewport.h"

class DoomVizEditor : public juce::AudioProcessorEditor
{
public:
    explicit DoomVizEditor(DoomVizProcessor&);
    ~DoomVizEditor() override;

    void resized() override;

private:
    DoomVizProcessor& processorRef;
    DoomViewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoomVizEditor)
};
