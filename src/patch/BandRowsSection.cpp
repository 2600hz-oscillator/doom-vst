#include "BandRowsSection.h"
#include "SpriteCatalog.h"

namespace patch
{

namespace
{
    constexpr int kRowHeight    = 26;
    constexpr int kHeaderHeight = 20;

    constexpr int kColLabelW  = 28;
    constexpr int kColHzW     = 60;
    constexpr int kColSpriteW = 130;
    constexpr int kColGapW    = 6;

    // ComboBox uses 1-based item IDs (0 == "no selection"); add a fixed
    // offset so the menu IDs are 1..N instead of 0..N-1.
    constexpr int kComboIdOffset = 1;

    // Doom HUD palette — same colours ControlPanel uses, kept local so the
    // section can be styled identically without leaking the palette through
    // a header.
    constexpr auto kStoneDark   = juce::uint32 { 0xff2a1f15 };
    constexpr auto kStoneLight  = juce::uint32 { 0xff3a2f25 };
    constexpr auto kBevelHi     = juce::uint32 { 0xffc8b070 };
    constexpr auto kBevelLo     = juce::uint32 { 0xff1a0f08 };
    constexpr auto kAccentRed   = juce::uint32 { 0xffb81818 };
    constexpr auto kGoldText    = juce::uint32 { 0xfff0d878 };
    constexpr auto kDimGold     = juce::uint32 { 0xff786850 };

    void populateSpriteCombo(juce::ComboBox& combo)
    {
        SpriteCategory currentCat = SpriteCategory::Character;
        bool first = true;
        for (size_t i = 0; i < kSpriteCatalog.size(); ++i)
        {
            const auto& e = kSpriteCatalog[i];
            if (first || e.category != currentCat)
            {
                const char* heading =
                    e.category == SpriteCategory::Character ? "Characters"
                  : e.category == SpriteCategory::Gun       ? "Guns"
                  : e.category == SpriteCategory::Powerup   ? "Powerups"
                  :                                           "Armor";
                combo.addSectionHeading(heading);
                currentCat = e.category;
                first = false;
            }
            combo.addItem(e.niceName, static_cast<int>(i) + kComboIdOffset);
        }
    }

    int spriteIdToComboItemId(int spriteId)
    {
        for (size_t i = 0; i < kSpriteCatalog.size(); ++i)
            if (kSpriteCatalog[i].id == spriteId)
                return static_cast<int>(i) + kComboIdOffset;
        return 0;
    }

    int comboItemIdToSpriteId(int itemId, int fallback)
    {
        int idx = itemId - kComboIdOffset;
        if (idx < 0 || idx >= static_cast<int>(kSpriteCatalog.size()))
            return fallback;
        return kSpriteCatalog[static_cast<size_t>(idx)].id;
    }
}

BandRowsSection::BandRowsSection()
{
    auto fireEdit = [this]() { if (onEdit) onEdit(); };

    auto styleHeader = [this](juce::Label& l, const juce::String& text)
    {
        l.setText(text, juce::dontSendNotification);
        l.setColour(juce::Label::textColourId, juce::Colour(kDimGold));
        l.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
        l.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(l);
    };
    styleHeader(headerBand,   "BAND");
    styleHeader(headerLow,    "LOW HZ");
    styleHeader(headerHigh,   "HIGH HZ");
    styleHeader(headerGain,   "GAIN");
    styleHeader(headerSprite, "SPRITE");

    for (int i = 0; i < kNumBands; ++i)
    {
        auto& r = rows[static_cast<size_t>(i)];

        r.nameLabel.setText(juce::String(i + 1), juce::dontSendNotification);
        r.nameLabel.setColour(juce::Label::textColourId, juce::Colour(kGoldText));
        r.nameLabel.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::bold));
        r.nameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(r.nameLabel);

        auto setupHzEdit = [&, fireEdit](juce::TextEditor& te)
        {
            te.setInputRestrictions(7, "0123456789.");
            te.setJustification(juce::Justification::centredRight);
            te.setColour(juce::TextEditor::backgroundColourId, juce::Colour(kBevelLo));
            te.setColour(juce::TextEditor::textColourId, juce::Colour(kGoldText));
            te.setColour(juce::TextEditor::outlineColourId, juce::Colour(kBevelHi));
            te.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(kAccentRed));
            te.onTextChange = fireEdit;
            addAndMakeVisible(te);
        };
        setupHzEdit(r.lowHz);
        setupHzEdit(r.highHz);

        r.gain.setSliderStyle(juce::Slider::LinearHorizontal);
        r.gain.setRange(0.0, 1.0, 0.001);
        r.gain.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        r.gain.setColour(juce::Slider::trackColourId,      juce::Colour(kAccentRed));
        r.gain.setColour(juce::Slider::backgroundColourId, juce::Colour(kBevelLo));
        r.gain.setColour(juce::Slider::thumbColourId,      juce::Colour(kGoldText));
        r.gain.onValueChange = fireEdit;
        addAndMakeVisible(r.gain);

        r.sprite.setColour(juce::ComboBox::backgroundColourId, juce::Colour(kStoneLight));
        r.sprite.setColour(juce::ComboBox::textColourId,       juce::Colour(kGoldText));
        r.sprite.setColour(juce::ComboBox::outlineColourId,    juce::Colour(kBevelHi));
        r.sprite.setColour(juce::ComboBox::arrowColourId,      juce::Colour(kGoldText));
        populateSpriteCombo(r.sprite);
        r.sprite.onChange = fireEdit;
        addAndMakeVisible(r.sprite);
    }
}

void BandRowsSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kStoneDark));
}

void BandRowsSection::resized()
{
    auto area = getLocalBounds();

    auto headerRow = area.removeFromTop(kHeaderHeight);
    headerBand.setBounds(headerRow.removeFromLeft(kColLabelW));
    headerRow.removeFromLeft(kColGapW);
    headerLow.setBounds(headerRow.removeFromLeft(kColHzW));
    headerRow.removeFromLeft(kColGapW);
    headerHigh.setBounds(headerRow.removeFromLeft(kColHzW));
    headerRow.removeFromLeft(kColGapW);
    headerSprite.setBounds(headerRow.removeFromRight(kColSpriteW));
    headerRow.removeFromRight(kColGapW);
    headerGain.setBounds(headerRow);

    area.removeFromTop(2);

    for (int i = 0; i < kNumBands; ++i)
    {
        auto& r = rows[static_cast<size_t>(i)];
        auto row = area.removeFromTop(kRowHeight);
        row.removeFromBottom(2);

        r.nameLabel.setBounds(row.removeFromLeft(kColLabelW));
        row.removeFromLeft(kColGapW);
        r.lowHz.setBounds(row.removeFromLeft(kColHzW));
        row.removeFromLeft(kColGapW);
        r.highHz.setBounds(row.removeFromLeft(kColHzW));
        row.removeFromLeft(kColGapW);
        r.sprite.setBounds(row.removeFromRight(kColSpriteW));
        row.removeFromRight(kColGapW);
        r.gain.setBounds(row);
    }
}

void BandRowsSection::loadFromState(const GlobalConfig& g)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        auto& r = rows[static_cast<size_t>(i)];
        // dontSendNotification so onTextChange / onValueChange don't fire and
        // re-set the dirty flag while we're loading.
        r.lowHz.setText(juce::String(g.bands[static_cast<size_t>(i)].lowHz, 1),
                        juce::dontSendNotification);
        r.highHz.setText(juce::String(g.bands[static_cast<size_t>(i)].highHz, 1),
                         juce::dontSendNotification);
        r.gain.setValue(static_cast<double>(g.bands[static_cast<size_t>(i)].gain01),
                        juce::dontSendNotification);
        int comboId = spriteIdToComboItemId(g.bands[static_cast<size_t>(i)].spriteId);
        if (comboId > 0)
            r.sprite.setSelectedId(comboId, juce::dontSendNotification);
    }
}

GlobalConfig BandRowsSection::buildConfig(const GlobalConfig& fallback) const
{
    GlobalConfig out = fallback;
    for (int i = 0; i < kNumBands; ++i)
    {
        const auto& r = rows[static_cast<size_t>(i)];
        float lo = r.lowHz.getText().getFloatValue();
        float hi = r.highHz.getText().getFloatValue();
        if (lo < 1.0f)     lo = 1.0f;
        if (hi > 22050.0f) hi = 22050.0f;
        if (hi <= lo)      hi = lo + 1.0f;
        out.bands[static_cast<size_t>(i)].lowHz  = lo;
        out.bands[static_cast<size_t>(i)].highHz = hi;
        out.bands[static_cast<size_t>(i)].gain01 =
            juce::jlimit(0.0f, 1.0f, static_cast<float>(r.gain.getValue()));
        out.bands[static_cast<size_t>(i)].spriteId =
            comboItemIdToSpriteId(r.sprite.getSelectedId(),
                                  out.bands[static_cast<size_t>(i)].spriteId);
    }
    return out;
}

} // namespace patch
