#include "ControlWindow.h"

// --- Doom HUD palette ---
// Pulled from Doom's actual status-bar look: dark warm-brown stone for the
// background, gold bevels on top/left edges, near-black on bottom/right,
// blood red for the active accent, and gold readouts for text.
namespace
{
    constexpr auto kStoneDark   = juce::uint32 { 0xff2a1f15 };
    constexpr auto kStoneLight  = juce::uint32 { 0xff3a2f25 };
    constexpr auto kBevelHi     = juce::uint32 { 0xffc8b070 };
    constexpr auto kBevelLo     = juce::uint32 { 0xff1a0f08 };
    constexpr auto kAccentRed   = juce::uint32 { 0xffb81818 };
    constexpr auto kTitleRed    = juce::uint32 { 0xffe02020 };
    constexpr auto kGoldText    = juce::uint32 { 0xfff0d878 };
    constexpr auto kDimGold     = juce::uint32 { 0xff786850 };
    constexpr auto kBlackText   = juce::uint32 { 0xff050200 };
}

// --- ControlPanel ---

ControlPanel::ControlPanel()
{
    auto setupButton = [this](juce::TextButton& btn, int index)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(kStoneLight));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(kDimGold));
        btn.onClick = [this, index]() { selectScene(index); };
        addAndMakeVisible(btn);
    };

    setupButton(sceneA, 0);
    setupButton(sceneB, 1);
    setupButton(sceneC, 2);

    sceneLabel.setColour(juce::Label::textColourId, juce::Colour(kDimGold));
    sceneLabel.setFont(juce::FontOptions("Courier New", 14.0f, juce::Font::bold));
    addAndMakeVisible(sceneLabel);

    activeLabel.setColour(juce::Label::textColourId, juce::Colour(kGoldText));
    activeLabel.setFont(juce::FontOptions("Courier New", 18.0f, juce::Font::bold));
    addAndMakeVisible(activeLabel);

    // Highlight initial scene (Kill Room) in red
    sceneA.setColour(juce::TextButton::buttonColourId, juce::Colour(kAccentRed));
    sceneA.setColour(juce::TextButton::textColourOffId, juce::Colour(kBlackText));

    fullscreenBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(kStoneLight));
    fullscreenBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kGoldText));
    fullscreenBtn.onClick = [this]()
    {
        isFullscreen = !isFullscreen;
        fullscreenBtn.setButtonText(isFullscreen ? "EXIT FULLSCREEN" : "FULLSCREEN");
        if (onToggleFullscreen)
            onToggleFullscreen(isFullscreen);
    };
    fullscreenBtn.setButtonText("FULLSCREEN");
    addAndMakeVisible(fullscreenBtn);

    patchBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(kStoneLight));
    patchBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kGoldText));
    patchBtn.onClick = [this]()
    {
        if (onTogglePatchSettings) onTogglePatchSettings();
    };
    patchBtn.setButtonText("PATCH SETTINGS");
    addAndMakeVisible(patchBtn);

    displayLabel.setColour(juce::Label::textColourId, juce::Colour(kDimGold));
    displayLabel.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
    displayLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(displayLabel);

    setSize(360, 280);
}

void ControlPanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();

    // Stone background.
    g.fillAll(juce::Colour(kStoneDark));

    // Outer bevel: 2px inset, gold top+left, near-black bottom+right.
    auto drawBevel = [&](juce::Rectangle<int> r, juce::Colour hi, juce::Colour lo, int thick)
    {
        for (int i = 0; i < thick; ++i)
        {
            g.setColour(hi);
            g.fillRect(r.getX(), r.getY() + i, r.getWidth() - i, 1);                       // top
            g.fillRect(r.getX() + i, r.getY(), 1, r.getHeight() - i);                      // left
            g.setColour(lo);
            g.fillRect(r.getX() + i, r.getBottom() - 1 - i, r.getWidth() - i, 1);          // bottom
            g.fillRect(r.getRight() - 1 - i, r.getY() + i, 1, r.getHeight() - i);          // right
        }
    };
    drawBevel(b, juce::Colour(kBevelHi), juce::Colour(kBevelLo), 2);

    // Title strip across the top (DOOMVIZ in red on dark, with bevel underline).
    auto title = b.reduced(2).removeFromTop(32);
    g.setColour(juce::Colour(0xff15100a));
    g.fillRect(title);
    g.setColour(juce::Colour(kTitleRed));
    g.setFont(juce::FontOptions("Courier New", 22.0f, juce::Font::bold));
    g.drawText("D O O M V I Z", title, juce::Justification::centred, false);
    // Sub-bevel under title strip.
    g.setColour(juce::Colour(kBevelLo));
    g.fillRect(title.getX(), title.getBottom(), title.getWidth(), 1);
    g.setColour(juce::Colour(kBevelHi));
    g.fillRect(title.getX(), title.getBottom() + 1, title.getWidth(), 1);
}

void ControlPanel::resized()
{
    // Reserve the top 32 px for the painted title strip; the rest is content.
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(32);     // skip painted title strip
    area.removeFromTop(8);      // breathing room under the bevel-line

    auto topRow = area.removeFromTop(28);
    sceneLabel.setBounds(topRow.removeFromLeft(60));
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
    area.removeFromTop(8);
    patchBtn.setBounds(area.removeFromTop(36));
    area.removeFromTop(10);
    displayLabel.setBounds(area.removeFromTop(16));
}

void ControlPanel::selectScene(int index)
{
    // Reset all button colors
    auto resetBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(kStoneLight));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(kDimGold));
    };
    resetBtn(sceneA);
    resetBtn(sceneB);
    resetBtn(sceneC);

    // Highlight selected
    auto highlightBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(kAccentRed));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(kBlackText));
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

void ControlPanel::setDisplayName(const juce::String& name)
{
    displayLabel.setText("Display: " + name, juce::dontSendNotification);
}

void ControlPanel::setActiveScene(int sceneIndex)
{
    // Same UI update as selectScene, minus firing onSceneChange — used to
    // reflect non-GUI scene switches (e.g. MIDI Program Change) back into
    // the button highlighting and active-label.
    auto resetBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(kStoneLight));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(kDimGold));
    };
    auto highlightBtn = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(kAccentRed));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(kBlackText));
    };
    resetBtn(sceneA); resetBtn(sceneB); resetBtn(sceneC);

    const char* names[] = { "Kill Room", "Analyzer", "Doom Spectrum" };
    switch (sceneIndex)
    {
        case 0: highlightBtn(sceneA); break;
        case 1: highlightBtn(sceneB); break;
        case 2: highlightBtn(sceneC); break;
        default: break;
    }
    if (sceneIndex >= 0 && sceneIndex < 3)
        activeLabel.setText(names[sceneIndex], juce::dontSendNotification);
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
