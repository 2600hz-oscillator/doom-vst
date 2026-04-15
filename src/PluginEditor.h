#pragma once

#include "PluginProcessor.h"
#include "DoomViewport.h"
#include "ControlWindow.h"
#include <memory>

class DoomVizEditor : public juce::AudioProcessorEditor
{
public:
    explicit DoomVizEditor(DoomVizProcessor&);
    ~DoomVizEditor() override;

    void resized() override;

private:
    DoomVizProcessor& processorRef;
    DoomViewport viewport;
    std::unique_ptr<ControlWindow> controlWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoomVizEditor)
};
