#include "BandAnalyzer.h"
#include "AudioAnalyzer.h"
#include <algorithm>
#include <cmath>

void BandAnalyzer::update(const AudioAnalyzer& analyzer,
                          const patch::GlobalConfig& global,
                          float deltaTime)
{
    const float* spectrum = analyzer.getMagnitudeSpectrum();
    const int    specSize = analyzer.getSpectrumSize();
    const double sampleRate = analyzer.getSampleRate();
    const float  binWidth = static_cast<float>(sampleRate) /
                            static_cast<float>(AudioAnalyzer::kFFTSize);

    std::array<float, kNumBands> rawBand {};
    for (int i = 0; i < kNumBands; ++i)
    {
        const auto& cfg = global.bands[static_cast<size_t>(i)];
        int lowBin  = std::max(1, static_cast<int>(cfg.lowHz  / binWidth));
        int highBin = std::min(specSize - 1, static_cast<int>(cfg.highHz / binWidth));
        if (highBin < lowBin) std::swap(lowBin, highBin);

        float sumSq = 0.0f;
        int count = 0;
        for (int bin = lowBin; bin <= highBin; ++bin)
        {
            float mag = spectrum[static_cast<size_t>(bin)];
            sumSq += mag * mag;
            count++;
        }
        rawBand[static_cast<size_t>(i)] =
            count > 0 ? std::sqrt(sumSq / static_cast<float>(count)) : 0.0f;
    }

    // Peak-normalize across the 8 bands so the loudest band lands ~1.0.
    float maxBand = *std::max_element(rawBand.begin(), rawBand.end());
    if (maxBand > 0.0001f)
    {
        for (auto& b : rawBand)
            b = std::min(1.0f, b / maxBand);
    }

    // Per-band envelope follower (one-pole, attack/release).
    float dt = std::max(0.001f, deltaTime);
    float attackCoeff  = 1.0f - std::exp(-dt / kAttack);
    float releaseCoeff = 1.0f - std::exp(-dt / kRelease);
    for (int i = 0; i < kNumBands; ++i)
    {
        float target = rawBand[static_cast<size_t>(i)];
        float& env = envelope[static_cast<size_t>(i)];
        env += (target > env ? attackCoeff : releaseCoeff) * (target - env);
    }
}
