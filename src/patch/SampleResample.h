#pragma once

#include <vector>

namespace patch
{

// Linear-Lagrange resample of `src` (mono samples at srcSr) to dstSr.
// Returns src unchanged when the rates match (within 0.5 Hz). Used by
// both the WAV decoder (LOAD button) and the .dvset loader so a sample
// set saved at 11 025 Hz plays correctly in a 48 kHz session.
std::vector<float> resampleMono(const std::vector<float>& src,
                                 double srcSr,
                                 double dstSr);

} // namespace patch
