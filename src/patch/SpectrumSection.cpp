#include "SpectrumSection.h"

namespace patch
{

namespace
{
    constexpr auto kStoneDark   = juce::uint32 { 0xff2a1f15 };
    constexpr auto kStoneLight  = juce::uint32 { 0xff3a2f25 };
    constexpr auto kBevelHi     = juce::uint32 { 0xffc8b070 };
    constexpr auto kBevelLo     = juce::uint32 { 0xff1a0f08 };
    constexpr auto kAccentRed   = juce::uint32 { 0xffb81818 };
    constexpr auto kGoldText    = juce::uint32 { 0xfff0d878 };
    constexpr auto kDimGold     = juce::uint32 { 0xff786850 };

    // Names mirror Spectrum2Scene::kDoomtexNames so the menu order maps 1:1
    // onto the texture index. Kept out of the header so renaming on either
    // side is easy.
    constexpr const char* kTexNames[8] = {
        "STARTAN3", "BROWN1",  "COMPSTA1", "COMPUTE1",
        "TEKWALL1", "BROWNHUG", "BIGDOOR2", "METAL"
    };

    void styleLabel(juce::Label& l, juce::Colour col, float size)
    {
        l.setColour(juce::Label::textColourId, col);
        l.setFont(juce::FontOptions("Courier New", size, juce::Font::bold));
        l.setJustificationType(juce::Justification::centredLeft);
    }

    void styleCombo(juce::ComboBox& c)
    {
        c.setColour(juce::ComboBox::backgroundColourId, juce::Colour(kStoneLight));
        c.setColour(juce::ComboBox::textColourId,       juce::Colour(kGoldText));
        c.setColour(juce::ComboBox::outlineColourId,    juce::Colour(kBevelHi));
        c.setColour(juce::ComboBox::arrowColourId,      juce::Colour(kGoldText));
    }
}

SpectrumSection::SpectrumSection()
{
    auto fireEdit = [this]() { if (onEdit) onEdit(); };

    styleLabel(vibeLabel, juce::Colour(kDimGold), 11.0f);
    addAndMakeVisible(vibeLabel);
    styleCombo(vibeCombo);
    static const char* kVibeNames[kNumBackgroundVibes] = {
        "ACIDWARP.EXE", "VAPORWAVE", "PUNKROCK", "DOOMTEX",
        "WINAMP", "STARFIELD", "CRTGLITCH"
    };
    for (int i = 0; i < kNumBackgroundVibes; ++i)
        vibeCombo.addItem(kVibeNames[i], i + 1);
    vibeCombo.onChange = fireEdit;
    addAndMakeVisible(vibeCombo);

    styleLabel(texLabel, juce::Colour(kDimGold), 11.0f);
    addAndMakeVisible(texLabel);
    styleCombo(texCombo);
    for (int i = 0; i < 8; ++i)
        texCombo.addItem(kTexNames[i], i + 1);
    texCombo.onChange = fireEdit;
    addAndMakeVisible(texCombo);

    autoAdvanceBtn.setColour(juce::ToggleButton::textColourId,    juce::Colour(kGoldText));
    autoAdvanceBtn.setColour(juce::ToggleButton::tickColourId,    juce::Colour(kAccentRed));
    autoAdvanceBtn.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(kBevelLo));
    autoAdvanceBtn.onClick = fireEdit;
    addAndMakeVisible(autoAdvanceBtn);

    styleLabel(autoBandLabel, juce::Colour(kDimGold), 11.0f);
    addAndMakeVisible(autoBandLabel);
    styleCombo(autoBandCombo);
    for (int i = 1; i <= kNumBands; ++i)
        autoBandCombo.addItem(juce::String(i), i);
    autoBandCombo.onChange = fireEdit;
    addAndMakeVisible(autoBandCombo);

    styleLabel(autoThreshLabel, juce::Colour(kDimGold), 11.0f);
    addAndMakeVisible(autoThreshLabel);
    autoThreshSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    autoThreshSlider.setRange(0.0, 1.0, 0.001);
    autoThreshSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    autoThreshSlider.setColour(juce::Slider::trackColourId,      juce::Colour(kAccentRed));
    autoThreshSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(kBevelLo));
    autoThreshSlider.setColour(juce::Slider::thumbColourId,      juce::Colour(kGoldText));
    autoThreshSlider.onValueChange = fireEdit;
    addAndMakeVisible(autoThreshSlider);
}

void SpectrumSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kStoneDark));
}

void SpectrumSection::resized()
{
    auto area = getLocalBounds();

    // Row 1: BACKGROUND vibe combo
    {
        auto row = area.removeFromTop(26);
        vibeLabel.setBounds(row.removeFromLeft(110));
        row.removeFromLeft(6);
        vibeCombo.setBounds(row.removeFromLeft(220));
    }
    area.removeFromTop(4);

    // Row 2: DOOMTEX texture combo + AUTO toggle
    {
        auto row = area.removeFromTop(26);
        texLabel.setBounds(row.removeFromLeft(110));
        row.removeFromLeft(6);
        texCombo.setBounds(row.removeFromLeft(140));
        row.removeFromLeft(8);
        autoAdvanceBtn.setBounds(row.removeFromLeft(80));
    }
    area.removeFromTop(4);

    // Row 3: BAND selector + THRESH slider
    {
        auto row = area.removeFromTop(26);
        autoBandLabel.setBounds(row.removeFromLeft(60));
        row.removeFromLeft(6);
        autoBandCombo.setBounds(row.removeFromLeft(60));
        row.removeFromLeft(20);
        autoThreshLabel.setBounds(row.removeFromLeft(60));
        row.removeFromLeft(6);
        autoThreshSlider.setBounds(row);
    }
}

void SpectrumSection::loadFromState(const SpectrumConfig& s)
{
    vibeCombo.setSelectedId(static_cast<int>(s.vibe) + 1, juce::dontSendNotification);
    texCombo.setSelectedId(juce::jlimit(0, 7, s.doomtexIndex) + 1,
                           juce::dontSendNotification);
    autoAdvanceBtn.setToggleState(s.doomtexAutoAdvance, juce::dontSendNotification);
    autoBandCombo.setSelectedId(juce::jlimit(1, kNumBands, s.doomtexAutoBand),
                                juce::dontSendNotification);
    autoThreshSlider.setValue(static_cast<double>(s.doomtexAutoThreshold),
                              juce::dontSendNotification);
}

SpectrumConfig SpectrumSection::buildConfig(const SpectrumConfig& fallback) const
{
    SpectrumConfig out = fallback;
    int vibeId = vibeCombo.getSelectedId();
    if (vibeId >= 1 && vibeId <= kNumBackgroundVibes)
        out.vibe = static_cast<BackgroundVibe>(vibeId - 1);

    int texId = texCombo.getSelectedId();
    if (texId >= 1 && texId <= 8)
        out.doomtexIndex = texId - 1;

    out.doomtexAutoAdvance = autoAdvanceBtn.getToggleState();

    int bandId = autoBandCombo.getSelectedId();
    if (bandId >= 1 && bandId <= kNumBands)
        out.doomtexAutoBand = bandId;

    out.doomtexAutoThreshold =
        juce::jlimit(0.0f, 1.0f, static_cast<float>(autoThreshSlider.getValue()));

    return out;
}

} // namespace patch
