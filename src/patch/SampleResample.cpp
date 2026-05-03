#include "SampleResample.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

namespace patch
{

std::vector<float> resampleMono(const std::vector<float>& src,
                                 double srcSr,
                                 double dstSr)
{
    if (src.empty() || srcSr <= 0.0 || dstSr <= 0.0) return src;
    if (std::abs(srcSr - dstSr) < 0.5) return src;

    const double ratio = srcSr / dstSr;
    const int    dstLen = static_cast<int>(
        std::ceil(static_cast<double>(src.size()) / ratio));
    if (dstLen <= 0) return {};

    std::vector<float> dst(static_cast<size_t>(dstLen), 0.0f);

    juce::Interpolators::Lagrange interp;
    interp.process(ratio, src.data(), dst.data(), dstLen);
    return dst;
}

} // namespace patch
