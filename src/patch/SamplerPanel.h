#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VisualizerState.h"

namespace patch
{

// DooMed SFX tab content. Phase 5 ships a placeholder header + status line;
// Phase 6 lands the 3×3 pad grid with LOAD button, Phase 7 adds the
// per-pad waveform + start/end markers.
class SamplerPanel : public juce::Component
{
public:
    explicit SamplerPanel(VisualizerState& state);

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VisualizerState& state;
    juce::Label header { {}, "-- DOOMED SFX --" };
    juce::Label status;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerPanel)
};

} // namespace patch
