#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SpectrumPatchPanel.h"
#include "PlaceholderPatchPanel.h"
#include <memory>

namespace patch
{

class PatchSettingsStore;

// Floating "Patch Settings" window. Displays a panel for the active scene.
// Only the Spectrum scene has real controls today; the others get a
// placeholder panel.
class PatchWindow : public juce::DocumentWindow
{
public:
    explicit PatchWindow(PatchSettingsStore& store);

    void closeButtonPressed() override;

    // Switch to the panel for scene index (0 = KillRoom, 1 = Analyzer,
    // 2 = Spectrum). Pending unsaved edits are silently reverted to match
    // the store's applied state.
    void setActiveScene(int sceneIndex);

private:
    PatchSettingsStore& store;

    std::unique_ptr<SpectrumPatchPanel>    spectrumPanel;
    std::unique_ptr<PlaceholderPatchPanel> killRoomPanel;
    std::unique_ptr<PlaceholderPatchPanel> analyzerPanel;

    int activeScene = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchWindow)
};

} // namespace patch
