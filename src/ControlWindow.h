#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

// Floating control panel in a separate OS window.
// Can be moved to a different display from the visualizer.
class ControlPanel : public juce::Component
{
public:
    ControlPanel();

    void paint(juce::Graphics&) override;
    void resized() override;

    // Callback when scene is selected (0, 1, 2)
    std::function<void(int)> onSceneChange;

    // Callback when the fullscreen toggle is clicked.
    std::function<void(bool)> onToggleFullscreen;

    // Externally update the fullscreen button's label/state without firing
    // the onToggleFullscreen callback. Used when the user exits fullscreen
    // via a different path (e.g. double-clicking the viewport).
    void setFullscreenState(bool fullscreen);

    // Update the small footer label that previews which display Fullscreen
    // will land on (driven by the editor polling its own screen bounds).
    void setDisplayName(const juce::String& name);

    // Programmatically select a scene without firing onSceneChange. Used by
    // the editor to keep button highlighting in sync when MIDI Program Change
    // (or any other non-GUI source) drives the scene switch.
    void setActiveScene(int sceneIndex);

    // Callback when the user clicks the "Patch Settings" toggle. The editor
    // owns the patch window and decides what to show.
    std::function<void()> onTogglePatchSettings;

private:
    juce::TextButton sceneA { "Kill Room" };
    juce::TextButton sceneB { "Analyzer" };
    juce::TextButton sceneC { "Doom Spectrum" };
    juce::TextButton fullscreenBtn { "Fullscreen" };
    juce::TextButton patchBtn { "Patch Settings" };
    juce::Label sceneLabel { {}, "Scene:" };
    juce::Label activeLabel { {}, "Kill Room" };
    juce::Label displayLabel { {}, "Display: -" };

    bool isFullscreen = false;

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
