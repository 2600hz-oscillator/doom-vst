#pragma once

#include "VisualizerState.h"
#include <juce_core/juce_core.h>
#include <optional>

namespace patch
{

// Save and load 9-pad sample sets as standalone `.dvset` XML files.
// The file embeds base64-encoded 16-bit PCM at each pad's stored
// `sourceSampleRate`. On load, sample buffers are resampled to the
// caller-supplied `hostSampleRate` so a set authored at 11 025 Hz
// (the Doom shareware sounds' native rate) plays correctly in any
// host SR.
class SampleSetIO
{
public:
    // Write `sm` as a `.dvset` file at `dst`. Overwrites if it exists.
    // Returns true on success.
    static bool saveToFile(const SamplerConfig& sm, const juce::File& dst);

    // Parse + resample-on-load. Returns nullopt on missing file,
    // malformed XML, or wrong root tag.
    static std::optional<SamplerConfig> loadFromFile(const juce::File& src,
                                                       double hostSampleRate);
};

} // namespace patch
