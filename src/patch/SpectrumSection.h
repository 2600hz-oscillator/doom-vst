#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VisualizerState.h"
#include <functional>

namespace patch
{

// Spectrum-scene-specific config: background vibe + DOOMTEX picker /
// auto-advance controls. Lives inside ControlPanel; not a window of its
// own.
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
    juce::Label    vibeLabel  { {}, "BACKGROUND" };
    juce::ComboBox vibeCombo;

    juce::Label    texLabel       { {}, "DOOMTEX" };
    juce::ComboBox texCombo;
    juce::ToggleButton autoAdvanceBtn { "AUTO" };

    juce::Label    autoBandLabel  { {}, "BAND" };
    juce::ComboBox autoBandCombo;

    juce::Label    autoThreshLabel { {}, "THRESH" };
    juce::Slider   autoThreshSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumSection)
};

} // namespace patch
