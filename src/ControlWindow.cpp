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

    constexpr int kTitleH       = 32;
    constexpr int kHeaderH      = 18;
    constexpr int kRowH         = 28;
    constexpr int kBtnH         = 32;
    constexpr int kSectionGap   = 10;
    constexpr int kPad          = 10;

    // 8 rows × 26 px row height + 20 px header + 2 px gap
    constexpr int kBandRowsH    = 20 + 2 + 8 * 26;

    // Spectrum section: vibe row + DOOMTEX row + auto-advance row. Each row
    // is 26 px high with a 4 px gap between them.
    constexpr int kSpectrumSectionH = 26 * 3 + 4 * 2;
    constexpr int kPlaceholderH     = 30;

    constexpr int kWindowW = 480;

    void styleSceneButton(juce::TextButton& btn, bool active)
    {
        if (active)
        {
            btn.setColour(juce::TextButton::buttonColourId,    juce::Colour(kAccentRed));
            btn.setColour(juce::TextButton::textColourOffId,   juce::Colour(kBlackText));
        }
        else
        {
            btn.setColour(juce::TextButton::buttonColourId,    juce::Colour(kStoneLight));
            btn.setColour(juce::TextButton::textColourOffId,   juce::Colour(kDimGold));
        }
    }
}

// --- ControlPanel ---

ControlPanel::ControlPanel(patch::VisualizerState& s)
    : state(s)
{
    auto styleHeader = [](juce::Label& l, juce::Colour col, float size, bool bold)
    {
        l.setColour(juce::Label::textColourId, col);
        l.setFont(juce::FontOptions("Courier New", size,
                                    bold ? juce::Font::bold : 0));
        l.setJustificationType(juce::Justification::centredLeft);
    };

    auto setupSceneButton = [this](juce::TextButton& btn, int index)
    {
        styleSceneButton(btn, false);
        btn.onClick = [this, index]() { selectScene(index); };
        addAndMakeVisible(btn);
    };
    setupSceneButton(sceneA, 0);
    setupSceneButton(sceneB, 1);
    setupSceneButton(sceneC, 2);
    styleSceneButton(sceneA, true);  // start with KillRoom highlighted

    styleHeader(sceneLabel,  juce::Colour(kDimGold),  14.0f, true);
    sceneLabel.setText("SCENE:", juce::dontSendNotification);
    addAndMakeVisible(sceneLabel);

    styleHeader(activeLabel, juce::Colour(kGoldText), 18.0f, true);
    activeLabel.setText("KILL ROOM", juce::dontSendNotification);
    addAndMakeVisible(activeLabel);

    fullscreenBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(kStoneLight));
    fullscreenBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kGoldText));
    fullscreenBtn.onClick = [this]()
    {
        isFullscreen = !isFullscreen;
        fullscreenBtn.setButtonText(isFullscreen ? "EXIT FULLSCREEN" : "FULLSCREEN");
        if (onToggleFullscreen)
            onToggleFullscreen(isFullscreen);
    };
    addAndMakeVisible(fullscreenBtn);

    styleHeader(displayLabel, juce::Colour(kDimGold), 11.0f, true);
    displayLabel.setText("DISPLAY: -", juce::dontSendNotification);
    addAndMakeVisible(displayLabel);

    styleHeader(globalHeader, juce::Colour(kBevelHi), 13.0f, true);
    globalHeader.setText("-- GLOBAL --", juce::dontSendNotification);
    addAndMakeVisible(globalHeader);

    styleHeader(sceneHeader, juce::Colour(kBevelHi), 13.0f, true);
    sceneHeader.setText("-- KILL ROOM --", juce::dontSendNotification);
    addAndMakeVisible(sceneHeader);

    addAndMakeVisible(bandRows);
    addChildComponent(spectrumSection);
    addChildComponent(killRoomSection);
    addChildComponent(analyzerSection);

    bandRows.onEdit        = [this]() { markDirty(); };
    spectrumSection.onEdit = [this]() { markDirty(); };

    auto setupFooterButton = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId,    juce::Colour(kStoneLight));
        btn.setColour(juce::TextButton::textColourOffId,   juce::Colour(kGoldText));
        btn.setColour(juce::TextButton::textColourOnId,    juce::Colour(kDimGold));
    };
    setupFooterButton(applyBtn);
    setupFooterButton(revertBtn);
    applyBtn.setEnabled(false);
    revertBtn.setEnabled(false);
    applyBtn.onClick  = [this]() { applyChanges();  };
    revertBtn.onClick = [this]() { revertChanges(); };
    addAndMakeVisible(applyBtn);
    addAndMakeVisible(revertBtn);

    loadFromState();
    setActiveScene(0);

    // Final size: title + scene block + global section + active scene section
    // + footer + padding. Tuned to fit 8 band rows + the spectrum section
    // without scrolling.
    int contentH = kTitleH + 8                     // title strip + gap
                 + 28 + 10                          // scene label row
                 + kBtnH + 10                       // scene buttons
                 + kBtnH + 8                        // fullscreen
                 + 16 + kSectionGap                 // display label + gap
                 + kHeaderH + 4 + kBandRowsH        // GLOBAL section
                 + kSectionGap
                 + kHeaderH + 4 + kSpectrumSectionH // active scene section
                 + kSectionGap
                 + kBtnH                            // footer apply/revert
                 + kPad;                            // bottom pad
    setSize(kWindowW, contentH);
}

juce::Component* ControlPanel::activeSection()
{
    switch (activeScene)
    {
        case 0:  return &killRoomSection;
        case 1:  return &analyzerSection;
        case 2:  return &spectrumSection;
        default: return &killRoomSection;
    }
}

void ControlPanel::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();
    g.fillAll(juce::Colour(kStoneDark));

    // Outer bevel.
    auto drawBevel = [&](juce::Rectangle<int> r, juce::Colour hi, juce::Colour lo, int thick)
    {
        for (int i = 0; i < thick; ++i)
        {
            g.setColour(hi);
            g.fillRect(r.getX(), r.getY() + i, r.getWidth() - i, 1);
            g.fillRect(r.getX() + i, r.getY(), 1, r.getHeight() - i);
            g.setColour(lo);
            g.fillRect(r.getX() + i, r.getBottom() - 1 - i, r.getWidth() - i, 1);
            g.fillRect(r.getRight() - 1 - i, r.getY() + i, 1, r.getHeight() - i);
        }
    };
    drawBevel(b, juce::Colour(kBevelHi), juce::Colour(kBevelLo), 2);

    // Title strip.
    auto title = b.reduced(2).removeFromTop(kTitleH);
    g.setColour(juce::Colour(0xff15100a));
    g.fillRect(title);
    g.setColour(juce::Colour(kTitleRed));
    g.setFont(juce::FontOptions("Courier New", 22.0f, juce::Font::bold));
    g.drawText("D O O M V I Z", title, juce::Justification::centred, false);
    g.setColour(juce::Colour(kBevelLo));
    g.fillRect(title.getX(), title.getBottom(),     title.getWidth(), 1);
    g.setColour(juce::Colour(kBevelHi));
    g.fillRect(title.getX(), title.getBottom() + 1, title.getWidth(), 1);
}

void ControlPanel::resized()
{
    auto area = getLocalBounds().reduced(kPad);
    area.removeFromTop(kTitleH);   // skip painted title strip
    area.removeFromTop(8);

    auto sceneRow = area.removeFromTop(28);
    sceneLabel.setBounds(sceneRow.removeFromLeft(70));
    activeLabel.setBounds(sceneRow);

    area.removeFromTop(10);

    auto btnRow = area.removeFromTop(kBtnH);
    int btnW = (btnRow.getWidth() - 10) / 3;
    sceneA.setBounds(btnRow.removeFromLeft(btnW));
    btnRow.removeFromLeft(5);
    sceneB.setBounds(btnRow.removeFromLeft(btnW));
    btnRow.removeFromLeft(5);
    sceneC.setBounds(btnRow);

    area.removeFromTop(10);
    fullscreenBtn.setBounds(area.removeFromTop(kBtnH));
    area.removeFromTop(8);
    displayLabel.setBounds(area.removeFromTop(16));

    area.removeFromTop(kSectionGap);

    globalHeader.setBounds(area.removeFromTop(kHeaderH));
    area.removeFromTop(4);
    bandRows.setBounds(area.removeFromTop(kBandRowsH));

    area.removeFromTop(kSectionGap);

    sceneHeader.setBounds(area.removeFromTop(kHeaderH));
    area.removeFromTop(4);
    auto sectionBounds = area.removeFromTop(kSpectrumSectionH);
    spectrumSection.setBounds(sectionBounds);
    killRoomSection.setBounds(sectionBounds);
    analyzerSection.setBounds(sectionBounds);

    area.removeFromTop(kSectionGap);

    auto footer = area.removeFromTop(kBtnH);
    revertBtn.setBounds(footer.removeFromLeft(100));
    applyBtn.setBounds(footer.removeFromRight(100));
}

void ControlPanel::selectScene(int index)
{
    setActiveScene(index);
    if (onSceneChange)
        onSceneChange(index);
}

void ControlPanel::highlightScene(int index)
{
    styleSceneButton(sceneA, index == 0);
    styleSceneButton(sceneB, index == 1);
    styleSceneButton(sceneC, index == 2);

    static const char* kSceneLabels[]  = { "KILL ROOM", "ANALYZER", "DOOM SPECTRUM" };
    static const char* kSectionLabels[]= { "-- KILL ROOM --", "-- ANALYZER --", "-- DOOM SPECTRUM --" };
    if (index >= 0 && index < 3)
    {
        activeLabel.setText(kSceneLabels[index],  juce::dontSendNotification);
        sceneHeader.setText(kSectionLabels[index], juce::dontSendNotification);
    }
}

void ControlPanel::setActiveScene(int sceneIndex)
{
    if (sceneIndex < 0 || sceneIndex > 2) return;

    // Switching scenes with pending edits silently reverts (per spec) so the
    // user doesn't accidentally overwrite scene-A config from scene-B's UI.
    if (dirty)
        revertChanges();

    activeScene = sceneIndex;
    highlightScene(sceneIndex);

    // Show only the matching section component.
    spectrumSection.setVisible(sceneIndex == 2);
    killRoomSection.setVisible(sceneIndex == 0);
    analyzerSection.setVisible(sceneIndex == 1);
}

void ControlPanel::setFullscreenState(bool fullscreen)
{
    if (isFullscreen == fullscreen)
        return;
    isFullscreen = fullscreen;
    fullscreenBtn.setButtonText(isFullscreen ? "EXIT FULLSCREEN" : "FULLSCREEN");
}

void ControlPanel::setDisplayName(const juce::String& name)
{
    displayLabel.setText("DISPLAY: " + name, juce::dontSendNotification);
}

void ControlPanel::markDirty()
{
    dirty = true;
    applyBtn.setEnabled(true);
    revertBtn.setEnabled(true);
}

void ControlPanel::applyChanges()
{
    auto g = bandRows.buildConfig(state.getGlobal());
    state.setGlobal(g);

    // Per-scene state. Only Spectrum has editable fields right now.
    auto s = spectrumSection.buildConfig(state.getSpectrum());
    state.setSpectrum(s);

    // Reflect any clamping back into the UI so what's shown matches what
    // the renderer actually got.
    bandRows.loadFromState(g);
    spectrumSection.loadFromState(s);

    dirty = false;
    applyBtn.setEnabled(false);
    revertBtn.setEnabled(false);
}

void ControlPanel::revertChanges()
{
    loadFromState();
    dirty = false;
    applyBtn.setEnabled(false);
    revertBtn.setEnabled(false);
}

void ControlPanel::loadFromState()
{
    bandRows.loadFromState(state.getGlobal());
    spectrumSection.loadFromState(state.getSpectrum());
}

// --- ControlWindow ---

ControlWindow::ControlWindow(patch::VisualizerState& s)
    : DocumentWindow("DoomViz Controls",
                     juce::Colour(0xff1a1a1a),
                     DocumentWindow::closeButton | DocumentWindow::minimiseButton),
      panel(s)
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
