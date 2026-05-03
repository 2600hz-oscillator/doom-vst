#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "VisualizerState.h"
#include "PadCell.h"
#include <array>
#include <functional>
#include <memory>

namespace patch
{

// DooMed SFX tab content. 3×3 pad grid with LOAD button per pad. LOAD
// opens a JUCE FileChooser defaulted to the bundled Doom shareware
// sounds; the picked WAV is decoded + resampled to the host SR + mono-
// downmixed and committed straight to VisualizerState. Phase 7 will
// add the per-pad waveform display + draggable start/end markers.
class SamplerPanel : public juce::Component
{
public:
    // `getHostSampleRate` is queried at LOAD time so the decoder can
    // resample to the running DAW's sample rate. `triggerPad` fires a
    // root-pitch one-shot when the user clicks a pad's TRIG button;
    // it must be safe to call from the GUI thread.
    SamplerPanel(VisualizerState& state,
                 std::function<double()> getHostSampleRate,
                 std::function<void(int)> triggerPad);

    void paint(juce::Graphics&) override;
    void resized() override;

    // Refresh the cells from the current persisted state (called on
    // construction, on revert, and when the SFX tab becomes active).
    void loadFromState(const SamplerConfig& s);

private:
    VisualizerState& state;
    std::function<double()> getHostSampleRate;
    std::function<void(int)> triggerPad;

    std::array<PadCell, kNumPads> cells {
        PadCell{0}, PadCell{1}, PadCell{2},
        PadCell{3}, PadCell{4}, PadCell{5},
        PadCell{6}, PadCell{7}, PadCell{8}
    };

    juce::Label header { {}, "-- DOOMED SFX --" };
    juce::Label hint;

    // Held while a FileChooser is open; the async callback fires after
    // the chooser dialog closes.
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Owns the format manager so reader creation is fast on repeated loads.
    juce::AudioFormatManager formatManager;

    void handleLoadPad(int padIndex);
    void commitPad(int padIndex, juce::String name,
                   double srcSampleRate, std::vector<float> data);

    // Resolve the bundled sounds dir; same lookup pattern as DOOM1.WAD
    // resolution in DoomViewport.
    static juce::File findBundledSoundsDir();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerPanel)
};

} // namespace patch
