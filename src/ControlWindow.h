#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "patch/VisualizerState.h"
#include "patch/BandRowsSection.h"
#include "patch/SpectrumSection.h"
#include "patch/PlaceholderSection.h"
#include <functional>
#include <memory>

// Floating control panel in a separate OS window. Hosts the visualizer's
// global band config + the active scene's per-scene config in a single,
// stacked layout (no separate "Patch Settings" window).
class ControlPanel : public juce::Component
{
public:
    explicit ControlPanel(patch::VisualizerState& state);

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
    // (or any other non-GUI source) drives the scene switch. Pending unsaved
    // edits in the per-scene section are silently reverted.
    void setActiveScene(int sceneIndex);

private:
    patch::VisualizerState& state;

    juce::TextButton sceneA { "KILL ROOM" };
    juce::TextButton sceneB { "ANALYZER" };
    juce::TextButton sceneC { "DOOM SPECTRUM" };
    juce::TextButton fullscreenBtn { "FULLSCREEN" };
    juce::TextButton applyBtn { "APPLY" };
    juce::TextButton revertBtn { "REVERT" };
    juce::Label sceneLabel { {}, "SCENE:" };
    juce::Label activeLabel { {}, "KILL ROOM" };
    juce::Label displayLabel { {}, "DISPLAY: -" };
    juce::Label globalHeader { {}, "GLOBAL" };
    juce::Label sceneHeader  { {}, "KILL ROOM" };

    patch::BandRowsSection bandRows;
    patch::SpectrumSection spectrumSection;
    patch::PlaceholderSection killRoomSection { "Kill Room" };
    patch::PlaceholderSection analyzerSection { "Analyzer" };

    int activeScene = 0;
    bool isFullscreen = false;
    bool dirty = false;

    juce::Component* activeSection();
    void selectScene(int index);
    void highlightScene(int index);
    void markDirty();
    void applyChanges();
    void revertChanges();
    void loadFromState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlPanel)
};

// Separate OS-level window that hosts the ControlPanel.
class ControlWindow : public juce::DocumentWindow
{
public:
    explicit ControlWindow(patch::VisualizerState& state);

    void closeButtonPressed() override;

    ControlPanel& getPanel() { return panel; }

private:
    ControlPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlWindow)
};
