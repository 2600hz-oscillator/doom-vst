#include "ControlWindow.h"

// --- ControlPanel ---

ControlPanel::ControlPanel()
{
    auto setupButton = [this](juce::TextButton& btn, int index)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
        btn.onClick = [this, index]() { selectScene(index); };
        addAndMakeVisible(btn);
    };

    setupButton(sceneA, 0);
    setupButton(sceneB, 1);
    setupButton(sceneC, 2);

    sceneLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    sceneLabel.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(sceneLabel);

    activeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff4444));
    activeLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(activeLabel);

    // Highlight initial scene
    sceneA.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff4444));
    sceneA.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));

    fullscreenBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
    fullscreenBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
    fullscreenBtn.onClick = [this]()
    {
        isFullscreen = !isFullscreen;
        fullscreenBtn.setButtonText(isFullscreen ? "Exit Fullscreen" : "Fullscreen");
        if (onToggleFullscreen)
            onToggleFullscreen(isFullscreen);
    };
    addAndMakeVisible(fullscreenBtn);

    patchBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
    patchBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
    patchBtn.onClick = [this]()
    {
        if (onTogglePatchSettings) onTogglePatchSettings();
    };
    addAndMakeVisible(patchBtn);

    setSize(340, 220);
}

void ControlPanel::resized()
{
    auto area = getLocalBounds().reduced(10);

    auto topRow = area.removeFromTop(24);
    sceneLabel.setBounds(topRow.removeFromLeft(50));
    activeLabel.setBounds(topRow);

    area.removeFromTop(10);

    auto btnRow = area.removeFromTop(36);
    int btnW = (btnRow.getWidth() - 10) / 3;
    sceneA.setBounds(btnRow.removeFromLeft(btnW));
    btnRow.removeFromLeft(5);
    sceneB.setBounds(btnRow.removeFromLeft(btnW));
    btnRow.removeFromLeft(5);
    sceneC.setBounds(btnRow);

    area.removeFromTop(10);
    fullscreenBtn.setBounds(area.removeFromTop(36));
    area.removeFromTop(10);
    patchBtn.setBounds(area.removeFromTop(36));
}

void ControlPanel::selectScene(int index)
{
    // Reset all button colors
    auto resetBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
    };
    resetBtn(sceneA);
    resetBtn(sceneB);
    resetBtn(sceneC);

    // Highlight selected
    auto highlightBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff4444));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));
    };

    const char* names[] = { "Kill Room", "Analyzer", "Doom Spectrum" };

    switch (index)
    {
        case 0: highlightBtn(sceneA); break;
        case 1: highlightBtn(sceneB); break;
        case 2: highlightBtn(sceneC); break;
    }

    if (index >= 0 && index < 3)
        activeLabel.setText(names[index], juce::dontSendNotification);

    if (onSceneChange)
        onSceneChange(index);
}

void ControlPanel::setFullscreenState(bool fullscreen)
{
    if (isFullscreen == fullscreen)
        return;
    isFullscreen = fullscreen;
    fullscreenBtn.setButtonText(isFullscreen ? "Exit Fullscreen" : "Fullscreen");
}

// --- ControlWindow ---

ControlWindow::ControlWindow()
    : DocumentWindow("DoomViz Controls",
                     juce::Colour(0xff1a1a1a),
                     DocumentWindow::closeButton | DocumentWindow::minimiseButton)
{
    setContentNonOwned(&panel, true);
    setResizable(false, false);
    setUsingNativeTitleBar(true);
    setAlwaysOnTop(true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void ControlWindow::closeButtonPressed()
{
    setVisible(false);
}
