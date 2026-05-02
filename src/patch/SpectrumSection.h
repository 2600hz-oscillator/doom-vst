#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VisualizerState.h"
#include <functional>

namespace patch
{

// Spectrum-scene-specific config. Phase 2: just the background-vibe combo.
// Phase 3 will add the DOOMTEX texture picker + auto-advance controls.
class SpectrumSection : public juce::Component
{
public:
    SpectrumSection();

    void paint(juce::Graphics&) override;
    void resized() override;

    void loadFromState(const SpectrumConfig& s);
    SpectrumConfig buildConfig(const SpectrumConfig& fallback) const;

    std::function<void()> onEdit;

private:
    juce::Label    vibeLabel { {}, "BACKGROUND" };
    juce::ComboBox vibeCombo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumSection)
};

} // namespace patch
