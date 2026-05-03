#include "PadCell.h"

namespace patch
{

namespace
{
    constexpr auto kStoneDark   = juce::uint32 { 0xff1a120a };
    constexpr auto kStoneLight  = juce::uint32 { 0xff3a2f25 };
    constexpr auto kBevelHi     = juce::uint32 { 0xffc8b070 };
    constexpr auto kBevelLo     = juce::uint32 { 0xff1a0f08 };
    constexpr auto kGoldText    = juce::uint32 { 0xfff0d878 };
    constexpr auto kDimGold     = juce::uint32 { 0xff786850 };
    constexpr auto kBlackText   = juce::uint32 { 0xff050200 };
}

PadCell::PadCell(int idx)
    : padIndex(idx)
{
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(kBevelHi));
    titleLabel.setFont(juce::FontOptions("Courier New", 12.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setText("PAD " + juce::String(padIndex + 1),
                       juce::dontSendNotification);
    addAndMakeVisible(titleLabel);

    nameLabel.setColour(juce::Label::textColourId, juce::Colour(kDimGold));
    nameLabel.setFont(juce::FontOptions("Courier New", 10.0f, juce::Font::italic));
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setText("—", juce::dontSendNotification);
    addAndMakeVisible(nameLabel);

    loadBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(kStoneLight));
    loadBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kGoldText));
    loadBtn.onClick = [this]() { if (onLoadClicked) onLoadClicked(); };
    addAndMakeVisible(loadBtn);

    // Trigger lights up red so it reads "this is the action" — matches
    // the active-scene button styling.
    trigBtn.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff782020));
    trigBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(kBlackText));
    trigBtn.setColour(juce::TextButton::textColourOnId,  juce::Colour(kBlackText));
    trigBtn.onClick = [this]() { if (onTriggerClicked) onTriggerClicked(); };
    trigBtn.setEnabled(false);          // gray until a sample is loaded
    addAndMakeVisible(trigBtn);

    waveform.onMarkersChanged = [this](int s, int e)
    {
        if (onMarkersChanged) onMarkersChanged(s, e);
    };
    addAndMakeVisible(waveform);
}

void PadCell::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();
    g.setColour(juce::Colour(kStoneDark));
    g.fillRect(b);

    // Recessed bevel: dark on top/left, gold on bottom/right.
    g.setColour(juce::Colour(kBevelLo));
    g.fillRect(b.getX(), b.getY(),                     b.getWidth(), 1);
    g.fillRect(b.getX(), b.getY(),                     1, b.getHeight());
    g.setColour(juce::Colour(kBevelHi).withAlpha(0.4f));
    g.fillRect(b.getX(), b.getBottom() - 1,            b.getWidth(), 1);
    g.fillRect(b.getRight() - 1, b.getY(),             1, b.getHeight());
}

void PadCell::resized()
{
    auto b = getLocalBounds().reduced(6);
    titleLabel.setBounds(b.removeFromTop(16));
    nameLabel .setBounds(b.removeFromTop(14));
    b.removeFromTop(4);

    // Bottom row: LOAD on the left, TRIG on the right.
    auto btnRow = b.removeFromBottom(22);
    const int half = btnRow.getWidth() / 2;
    loadBtn.setBounds(btnRow.removeFromLeft(half - 2));
    btnRow.removeFromLeft(4);
    trigBtn.setBounds(btnRow);
    b.removeFromBottom(4);

    // Whatever's left in the middle is the waveform area.
    waveform.setBounds(b);
}

void PadCell::setPad(const PadConfig& pad)
{
    nameLabel.setText(pad.name.isEmpty() ? "—" : pad.name,
                      juce::dontSendNotification);
    trigBtn.setEnabled(! pad.sampleData.empty());
    waveform.setPad(pad);
}

} // namespace patch
