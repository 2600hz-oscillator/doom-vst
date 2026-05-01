#include "PatchWindow.h"
#include "PatchSettingsStore.h"

namespace patch
{

PatchWindow::PatchWindow(PatchSettingsStore& s)
    : DocumentWindow("Patch Settings",
                     juce::Colour(0xff1a1a1a),
                     DocumentWindow::closeButton | DocumentWindow::minimiseButton),
      store(s)
{
    setUsingNativeTitleBar(true);
    setResizable(false, false);
    setAlwaysOnTop(true);

    spectrumPanel = std::make_unique<SpectrumPatchPanel>(store);
    killRoomPanel = std::make_unique<PlaceholderPatchPanel>("Kill Room");
    analyzerPanel = std::make_unique<PlaceholderPatchPanel>("Analyzer Room");

    setActiveScene(2);  // default to Spectrum (only scene with real controls)
    centreWithSize(getWidth(), getHeight());
    setVisible(false);
}

void PatchWindow::closeButtonPressed()
{
    setVisible(false);
}

void PatchWindow::setActiveScene(int sceneIndex)
{
    if (activeScene == sceneIndex && getContentComponent() != nullptr)
        return;

    activeScene = sceneIndex;

    juce::Component* panel = nullptr;
    switch (sceneIndex)
    {
        case 0:  panel = killRoomPanel.get(); break;
        case 1:  panel = analyzerPanel.get(); break;
        case 2:  panel = spectrumPanel.get(); break;
        default: panel = spectrumPanel.get(); break;
    }

    // Silently revert pending edits when switching panels (per spec).
    if (sceneIndex == 2 && spectrumPanel)
        spectrumPanel->loadFromStore();

    setContentNonOwned(panel, true);
}

} // namespace patch
