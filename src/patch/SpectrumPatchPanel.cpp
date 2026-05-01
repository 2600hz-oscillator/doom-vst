#include "SpectrumPatchPanel.h"
#include "PatchSettingsStore.h"
#include "SpriteCatalog.h"

namespace patch
{

namespace
{
    constexpr int kRowHeight = 28;
    constexpr int kHeaderHeight = 22;
    constexpr int kFooterHeight = 36;
    constexpr int kVibeRowHeight = 28;
    constexpr int kPad = 10;

    constexpr int kColLabelW  = 36;
    constexpr int kColHzW     = 70;
    constexpr int kColSpriteW = 150;
    constexpr int kColGapW    = 6;

    // ComboBox uses 1-based item IDs (0 == "no selection"); add a fixed
    // offset so the menu IDs are 1..N instead of 0..N-1.
    constexpr int kComboIdOffset = 1;

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
        return 0;  // not found
    }

    int comboItemIdToSpriteId(int itemId, int fallback)
    {
        int idx = itemId - kComboIdOffset;
        if (idx < 0 || idx >= static_cast<int>(kSpriteCatalog.size()))
            return fallback;
        return kSpriteCatalog[static_cast<size_t>(idx)].id;
    }
}

SpectrumPatchPanel::SpectrumPatchPanel(PatchSettingsStore& s)
    : store(s)
{
    auto styleHeader = [this](juce::Label& l, const juce::String& text)
    {
        l.setText(text, juce::dontSendNotification);
        l.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
        l.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        l.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(l);
    };
    styleHeader(headerBand,   "Band");
    styleHeader(headerLow,    "Low Hz");
    styleHeader(headerHigh,   "High Hz");
    styleHeader(headerGain,   "Gain");
    styleHeader(headerSprite, "Sprite");

    vibeLabel.setText("Background Vibe:", juce::dontSendNotification);
    vibeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
    vibeLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    vibeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(vibeLabel);

    vibeCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222222));
    vibeCombo.setColour(juce::ComboBox::textColourId, juce::Colour(0xffeeeeee));
    vibeCombo.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff444444));
    vibeCombo.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffcccccc));
    static const char* kVibeNames[kNumBackgroundVibes] = {
        "ACIDWARP.EXE", "VAPORWAVE", "PUNKROCK", "DOOMTEX",
        "WINAMP", "STARFIELD", "CRTGLITCH"
    };
    for (int i = 0; i < kNumBackgroundVibes; ++i)
        vibeCombo.addItem(kVibeNames[i], i + 1);
    vibeCombo.onChange = [this]() { markDirty(); };
    addAndMakeVisible(vibeCombo);

    for (int i = 0; i < kSpectrumNumBands; ++i)
    {
        auto& r = rows[static_cast<size_t>(i)];

        r.nameLabel.setText(juce::String(i + 1), juce::dontSendNotification);
        r.nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
        r.nameLabel.setFont(juce::FontOptions(13.0f));
        r.nameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(r.nameLabel);

        auto setupHzEdit = [this](juce::TextEditor& te)
        {
            te.setInputRestrictions(7, "0123456789.");
            te.setJustification(juce::Justification::centredRight);
            te.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff222222));
            te.setColour(juce::TextEditor::textColourId, juce::Colour(0xffeeeeee));
            te.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff444444));
            te.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xffff4444));
            te.onTextChange = [this]() { markDirty(); };
            addAndMakeVisible(te);
        };
        setupHzEdit(r.lowHz);
        setupHzEdit(r.highHz);

        r.gain.setSliderStyle(juce::Slider::LinearHorizontal);
        r.gain.setRange(0.0, 1.0, 0.001);
        r.gain.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        r.gain.setColour(juce::Slider::trackColourId, juce::Colour(0xffff4444));
        r.gain.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff333333));
        r.gain.setColour(juce::Slider::thumbColourId, juce::Colour(0xffeeeeee));
        r.gain.onValueChange = [this]() { markDirty(); };
        addAndMakeVisible(r.gain);

        r.sprite.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222222));
        r.sprite.setColour(juce::ComboBox::textColourId, juce::Colour(0xffeeeeee));
        r.sprite.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff444444));
        r.sprite.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffcccccc));
        populateSpriteCombo(r.sprite);
        r.sprite.onChange = [this]() { markDirty(); };
        addAndMakeVisible(r.sprite);
    }

    applyBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
    applyBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
    applyBtn.setEnabled(false);
    applyBtn.onClick = [this]() { applyChanges(); };
    addAndMakeVisible(applyBtn);

    revertBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
    revertBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
    revertBtn.setEnabled(false);
    revertBtn.onClick = [this]() { loadFromStore(); };
    addAndMakeVisible(revertBtn);

    // Drive the size of the hosting window via setContentNonOwned. Sized to
    // fit vibe row + header + 8 band rows + footer + padding.
    setSize(640, 376);

    loadFromStore();
}

void SpectrumPatchPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void SpectrumPatchPanel::resized()
{
    auto area = getLocalBounds().reduced(kPad);

    auto footer = area.removeFromBottom(kFooterHeight);
    area.removeFromBottom(8);

    // Background Vibe row at the top of the panel.
    auto vibeRow = area.removeFromTop(kVibeRowHeight);
    vibeLabel.setBounds(vibeRow.removeFromLeft(140));
    vibeRow.removeFromLeft(8);
    vibeCombo.setBounds(vibeRow.removeFromLeft(220));
    area.removeFromTop(8);

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

    area.removeFromTop(4);

    for (int i = 0; i < kSpectrumNumBands; ++i)
    {
        auto& r = rows[static_cast<size_t>(i)];
        auto row = area.removeFromTop(kRowHeight);
        row.removeFromBottom(4);

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

    // Apply button anchored to the lower-right per spec.
    applyBtn.setBounds(footer.removeFromRight(80));
    footer.removeFromRight(10);
    revertBtn.setBounds(footer.removeFromRight(80));
}

void SpectrumPatchPanel::markDirty()
{
    dirty = true;
    applyBtn.setEnabled(true);
    revertBtn.setEnabled(true);
}

SpectrumSettings SpectrumPatchPanel::buildSettingsFromUI() const
{
    SpectrumSettings out = store.getSpectrum();  // start from applied as fallback

    int vibeId = vibeCombo.getSelectedId();
    if (vibeId >= 1 && vibeId <= kNumBackgroundVibes)
        out.vibe = static_cast<BackgroundVibe>(vibeId - 1);

    for (int i = 0; i < kSpectrumNumBands; ++i)
    {
        const auto& r = rows[static_cast<size_t>(i)];
        float lo = r.lowHz.getText().getFloatValue();
        float hi = r.highHz.getText().getFloatValue();
        // Validate / clamp.
        if (lo < 1.0f)    lo = 1.0f;
        if (hi > 22050.0f) hi = 22050.0f;
        if (hi <= lo)     hi = lo + 1.0f;
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

void SpectrumPatchPanel::writeSettingsToUI(const SpectrumSettings& s)
{
    vibeCombo.setSelectedId(static_cast<int>(s.vibe) + 1, juce::dontSendNotification);

    for (int i = 0; i < kSpectrumNumBands; ++i)
    {
        auto& r = rows[static_cast<size_t>(i)];
        // dontSendNotification so onTextChange / onValueChange don't fire and
        // re-set the dirty flag while we're loading.
        r.lowHz.setText(juce::String(s.bands[static_cast<size_t>(i)].lowHz, 1),
                        juce::dontSendNotification);
        r.highHz.setText(juce::String(s.bands[static_cast<size_t>(i)].highHz, 1),
                         juce::dontSendNotification);
        r.gain.setValue(static_cast<double>(s.bands[static_cast<size_t>(i)].gain01),
                        juce::dontSendNotification);
        int comboId = spriteIdToComboItemId(s.bands[static_cast<size_t>(i)].spriteId);
        if (comboId > 0)
            r.sprite.setSelectedId(comboId, juce::dontSendNotification);
    }
}

void SpectrumPatchPanel::loadFromStore()
{
    writeSettingsToUI(store.getSpectrum());
    dirty = false;
    applyBtn.setEnabled(false);
    revertBtn.setEnabled(false);
}

void SpectrumPatchPanel::applyChanges()
{
    SpectrumSettings s = buildSettingsFromUI();
    store.setSpectrum(s);
    // Reflect any clamping back into the textboxes so the UI matches what
    // the renderer actually got.
    writeSettingsToUI(s);
    dirty = false;
    applyBtn.setEnabled(false);
    revertBtn.setEnabled(false);
    if (onApply) onApply();
}

} // namespace patch
