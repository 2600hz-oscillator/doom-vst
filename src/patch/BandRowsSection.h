#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VisualizerState.h"
#include <array>
#include <functional>

namespace patch
{

// Editable 8-band global config. One row per band with low-Hz/high-Hz text
// editors, a 0..1 gain slider, and a sprite-picker combo. Lives inline in
// ControlPanel; not a window of its own.
//
// Edits are *staged* in the UI — Apply/Revert in the parent ControlPanel
// commits or discards. onEdit fires whenever the user changes any field so
// the parent can light up the Apply button.
class BandRowsSection : public juce::Component
{
public:
    BandRowsSection();

    void paint(juce::Graphics&) override;
    void resized() override;

    // Read state into the widgets.
    void loadFromState(const GlobalConfig& g);

    // Build a GlobalConfig from current widget values, clamping ranges
    // (lowHz >= 1, highHz <= 22050, highHz > lowHz). `fallback` supplies
    // any value the UI doesn't represent (e.g. an unknown spriteId).
    GlobalConfig buildConfig(const GlobalConfig& fallback) const;

    // Fired when any control changes value.
    std::function<void()> onEdit;

private:
    struct BandRow
    {
        juce::Label    nameLabel;
        juce::TextEditor lowHz;
        juce::TextEditor highHz;
        juce::Slider   gain;
        juce::ComboBox sprite;
    };

    std::array<BandRow, kNumBands> rows;
    juce::Label headerBand, headerLow, headerHigh, headerGain, headerSprite;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandRowsSection)
};

} // namespace patch
