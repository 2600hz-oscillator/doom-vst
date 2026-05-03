#include "audio/AudioAnalyzer.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <vector>

namespace
{
    // POSIX exposes M_PI through <cmath>; MSVC requires _USE_MATH_DEFINES
    // before <math.h>. Defining a portable constant here is simpler than
    // guessing whether some upstream header set the flag.
    constexpr double kPi = 3.14159265358979323846;
}

namespace
{
    // Generates `numSamples` of a `freqHz` sine at `sampleRate` and pushes the
    // entire buffer through `analyzer` so the FFT window fills.
    void feedSine(AudioAnalyzer& analyzer, double sampleRate, double freqHz, int numSamples,
                  float amplitude = 0.5f)
    {
        std::vector<float> buf(numSamples);
        const double w = 2.0 * kPi * freqHz / sampleRate;
        for (int i = 0; i < numSamples; ++i)
            buf[i] = amplitude * static_cast<float>(std::sin(w * i));
        analyzer.pushSamples(buf.data(), numSamples);
    }
}

TEST_CASE("AudioAnalyzer: silence yields ~zero RMS and no spectrum energy", "[audio_analyzer]")
{
    AudioAnalyzer a;
    a.setSampleRate(44100.0);

    std::vector<float> zeros(AudioAnalyzer::kFFTSize, 0.0f);
    a.pushSamples(zeros.data(), (int) zeros.size());

    REQUIRE(a.getRMSLevel() < 1e-5f);

    const float* spec = a.getMagnitudeSpectrum();
    int spectrumSum = 0;
    for (int i = 0; i < a.getSpectrumSize(); ++i)
        if (spec[i] > 1e-5f) ++spectrumSum;
    REQUIRE(spectrumSum == 0);
}

TEST_CASE("AudioAnalyzer: 1 kHz sine concentrates energy in a 1 kHz FFT bin", "[audio_analyzer]")
{
    AudioAnalyzer a;
    constexpr double sr = 44100.0;
    a.setSampleRate(sr);

    feedSine(a, sr, 1000.0, AudioAnalyzer::kFFTSize, 0.5f);

    const float* spec = a.getMagnitudeSpectrum();
    const int specSize = a.getSpectrumSize();

    // Bin frequency = bin * sr / fftSize. Find the strongest bin and confirm
    // it sits within ~half a bin width of 1 kHz.
    int peakBin = 0;
    float peakMag = 0.0f;
    for (int i = 0; i < specSize; ++i)
    {
        if (spec[i] > peakMag) { peakMag = spec[i]; peakBin = i; }
    }
    const double peakFreq = peakBin * sr / AudioAnalyzer::kFFTSize;
    REQUIRE(peakMag > 0.0f);
    REQUIRE(std::abs(peakFreq - 1000.0) < sr / AudioAnalyzer::kFFTSize);
}

TEST_CASE("AudioAnalyzer: non-zero input produces non-zero RMS", "[audio_analyzer]")
{
    AudioAnalyzer a;
    a.setSampleRate(44100.0);
    feedSine(a, 44100.0, 440.0, AudioAnalyzer::kFFTSize, 0.3f);
    REQUIRE(a.getRMSLevel() > 0.0f);
}

TEST_CASE("AudioAnalyzer: pushSamples below the FFT window does not crash", "[audio_analyzer]")
{
    AudioAnalyzer a;
    a.setSampleRate(44100.0);
    std::vector<float> tiny(64, 0.0f);
    a.pushSamples(tiny.data(), (int) tiny.size());
    // Nothing assertable yet — the bar is just "no crash and no UB".
    SUCCEED();
}
