#pragma once

#include "PluginProcessor.h"
#include "DoomViewport.h"
#include "ControlWindow.h"
#include <memory>

class DoomVizEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit DoomVizEditor(DoomVizProcessor&);
    ~DoomVizEditor() override;

    void resized() override;

private:
    DoomVizProcessor& processorRef;
    DoomViewport viewport;
    std::unique_ptr<ControlWindow> controlWindow;
    std::unique_ptr<juce::DocumentWindow> fullscreenWindow;

    void enterFullscreen();
    void exitFullscreen();

    // Polls the editor's screen bounds and pushes the name of the display
    // that fullscreen would target to the control panel, so the user can
    // see where Fullscreen will land before clicking it.
    void timerCallback() override;
    juce::String describeTargetDisplay() const;
    juce::String lastDisplayName;
    int lastObservedScene = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoomVizEditor)
};
