#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VisualizerState.h"
#include <functional>
#include <vector>

namespace patch
{

// Per-pad waveform display with two draggable vertical handles for the
// playback start/end markers. Caches a copy of the pad's sample buffer
// so peak recomputation on resize works without going back to
// VisualizerState. Memory cost: ~5 MB per pad worst-case (30 sec at
// 48 kHz mono float); 9 pads × 5 MB ≈ 45 MB GUI-side. Acceptable
// for a sample player.
class WaveformView : public juce::Component
{
public:
    WaveformView();

    // Replace the displayed waveform + reset markers to full range.
    // Call with an empty `pad.sampleData` to clear the view.
    void setPad(const PadConfig& pad);

    // Fired live during drag with the new (start, end) sample positions.
    std::function<void(int newStart, int newEnd)> onMarkersChanged;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;

private:
    enum class DragTarget { None, Start, End };

    std::vector<float> sourceData;
    std::vector<std::pair<float, float>> peaks;
    bool peaksDirty { true };

    int totalSamples { 0 };
    int startSample  { 0 };
    int endSample    { 0 };

    DragTarget dragging { DragTarget::None };
    DragTarget hovered  { DragTarget::None };

    void recomputePeaks();
    int  xToSample(int x) const;
    int  sampleToX(int sample) const;
    DragTarget hitTestHandle(int x) const;
    void notifyMarkersChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformView)
};

} // namespace patch
