#include "SpectrumSection.h"

namespace patch
{

namespace
{
    constexpr auto kStoneDark   = juce::uint32 { 0xff2a1f15 };
    constexpr auto kStoneLight  = juce::uint32 { 0xff3a2f25 };
    constexpr auto kBevelHi     = juce::uint32 { 0xffc8b070 };
    constexpr auto kGoldText    = juce::uint32 { 0xfff0d878 };
    constexpr auto kDimGold     = juce::uint32 { 0xff786850 };
}

SpectrumSection::SpectrumSection()
{
    vibeLabel.setColour(juce::Label::textColourId, juce::Colour(kDimGold));
    vibeLabel.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
    vibeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(vibeLabel);

    vibeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(kStoneLight));
    vibeCombo.setColour(juce::ComboBox::textColourId,       juce::Colour(kGoldText));
    vibeCombo.setColour(juce::ComboBox::outlineColourId,    juce::Colour(kBevelHi));
    vibeCombo.setColour(juce::ComboBox::arrowColourId,      juce::Colour(kGoldText));
    static const char* kVibeNames[kNumBackgroundVibes] = {
        "ACIDWARP.EXE", "VAPORWAVE", "PUNKROCK", "DOOMTEX",
        "WINAMP", "STARFIELD", "CRTGLITCH"
    };
    for (int i = 0; i < kNumBackgroundVibes; ++i)
        vibeCombo.addItem(kVibeNames[i], i + 1);
    vibeCombo.onChange = [this]() { if (onEdit) onEdit(); };
    addAndMakeVisible(vibeCombo);
}

void SpectrumSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kStoneDark));
}

void SpectrumSection::resized()
{
    auto area = getLocalBounds();
    auto row = area.removeFromTop(26);
    vibeLabel.setBounds(row.removeFromLeft(110));
    row.removeFromLeft(6);
    vibeCombo.setBounds(row.removeFromLeft(220));
}

void SpectrumSection::loadFromState(const SpectrumConfig& s)
{
    vibeCombo.setSelectedId(static_cast<int>(s.vibe) + 1, juce::dontSendNotification);
}

SpectrumConfig SpectrumSection::buildConfig(const SpectrumConfig& fallback) const
{
    SpectrumConfig out = fallback;
    int vibeId = vibeCombo.getSelectedId();
    if (vibeId >= 1 && vibeId <= kNumBackgroundVibes)
        out.vibe = static_cast<BackgroundVibe>(vibeId - 1);
    return out;
}

} // namespace patch
