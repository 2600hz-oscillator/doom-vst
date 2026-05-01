#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PatchSettings.h"
#include <array>
#include <functional>

namespace patch
{

class PatchSettingsStore;

// Editable patch panel for the Spectrum scene. Each band has a low-Hz
// textbox, a high-Hz textbox, and a 0..1 slider that attenuates how strongly
// that band's audio level scales the sprite. Changes only take effect after
// the user clicks Apply.
class SpectrumPatchPanel : public juce::Component
{
public:
    explicit SpectrumPatchPanel(PatchSettingsStore& store);

    void paint(juce::Graphics&) override;
    void resized() override;

    // Reload widget values from the store (silently reverts pending edits).
    void loadFromStore();

    // Called when Apply is clicked, after the store has been updated. The
    // editor uses this to know to refocus or repaint.
    std::function<void()> onApply;

private:
    struct BandRow
    {
        juce::Label    nameLabel;
        juce::TextEditor lowHz;
        juce::TextEditor highHz;
        juce::Slider   gain;
        juce::ComboBox sprite;
    };

    PatchSettingsStore& store;
    std::array<BandRow, kSpectrumNumBands> rows;

    juce::Label headerBand, headerLow, headerHigh, headerGain, headerSprite;
    juce::TextButton applyBtn { "Apply" };
    juce::TextButton revertBtn { "Revert" };

    bool dirty = false;
    void markDirty();
    void applyChanges();

    SpectrumSettings buildSettingsFromUI() const;
    void writeSettingsToUI(const SpectrumSettings& s);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumPatchPanel)
};

} // namespace patch
