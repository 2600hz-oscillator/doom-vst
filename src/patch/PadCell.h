#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VisualizerState.h"
#include "WaveformView.h"

namespace patch
{

// One sampler pad (1 of 9). Phase 6 ships title + LOAD button. Phase 7
// will inject the waveform display + drag-handles between them.
class PadCell : public juce::Component
{
public:
    explicit PadCell(int padIndex);

    void paint(juce::Graphics&) override;
    void resized() override;

    // Fired when the user clicks LOAD.
    std::function<void()> onLoadClicked;

    // Fired when the user clicks TRIG (one-shot preview at root pitch).
    std::function<void()> onTriggerClicked;

    // Fired live during a marker drag.
    std::function<void(int newStart, int newEnd)> onMarkersChanged;

    // Refresh the cell from the matching pad's persisted config. Also
    // enables/disables the TRIG button based on whether a sample is
    // loaded.
    void setPad(const PadConfig& pad);

    int  getPadIndex() const { return padIndex; }

private:
    int padIndex;
    juce::Label      titleLabel;
    juce::Label      nameLabel;
    WaveformView     waveform;
    juce::TextButton loadBtn { "LOAD" };
    juce::TextButton trigBtn { "TRIG" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadCell)
};

} // namespace patch
