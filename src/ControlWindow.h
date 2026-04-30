#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// Floating control panel in a separate OS window.
// Can be moved to a different display from the visualizer.
class ControlPanel : public juce::Component
{
public:
    ControlPanel();

    void resized() override;

    // Callback when scene is selected (0, 1, 2)
    std::function<void(int)> onSceneChange;

private:
    juce::TextButton sceneA { "Kill Room" };
    juce::TextButton sceneB { "Analyzer" };
    juce::TextButton sceneC { "Acidwarp" };
    juce::Label sceneLabel { {}, "Scene:" };
    juce::Label activeLabel { {}, "Kill Room" };

    void selectScene(int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};

// Separate OS-level window that hosts the ControlPanel.
class ControlWindow : public juce::DocumentWindow
{
public:
    ControlWindow();

    void closeButtonPressed() override;

    ControlPanel& getPanel() { return panel; }

private:
    ControlPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlWindow)
};
