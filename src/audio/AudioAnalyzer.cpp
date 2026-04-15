#include "AudioAnalyzer.h"
#include <cmath>
#include <algorithm>

AudioAnalyzer::AudioAnalyzer()
    : fft(kFFTOrder),
      window(static_cast<size_t>(kFFTSize), juce::dsp::WindowingFunction<float>::hann),
      magnitudeSpectrum(static_cast<size_t>(kFFTSize / 2 + 1), 0.0f)
{
}

void AudioAnalyzer::pushSamples(const float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        fftInput[static_cast<size_t>(fftInputPos)] = data[i];
        fftInputPos++;

        if (fftInputPos >= kFFTSize)
        {
            fftInputPos = 0;
            fftReady = true;
        }
    }

    if (fftReady)
    {
        fftReady = false;
        processFFT();
        computeBands();
        updateEnvelopes();
        detectOnset();
    }
}

void AudioAnalyzer::processFFT()
{
    // Copy input and apply window
    std::copy(fftInput.begin(), fftInput.end(), fftData.begin());
    std::fill(fftData.begin() + kFFTSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable(fftData.data(), static_cast<size_t>(kFFTSize));
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Extract magnitudes and compute RMS
    float sumSq = 0.0f;
    const int specSize = kFFTSize / 2 + 1;
    const float invSize = 1.0f / static_cast<float>(kFFTSize);

    for (int i = 0; i < specSize; ++i)
    {
        float mag = fftData[static_cast<size_t>(i)] * invSize;
        magnitudeSpectrum[static_cast<size_t>(i)] = mag;
        sumSq += mag * mag;
    }

    rmsLevel = std::sqrt(sumSq / static_cast<float>(specSize));
}

void AudioAnalyzer::computeBands()
{
    const int specSize = kFFTSize / 2 + 1;
    const float binWidth = static_cast<float>(sampleRate) / static_cast<float>(kFFTSize);

    for (int band = 0; band < kNumBands; ++band)
    {
        float lowFreq = kBandEdges[band];
        float highFreq = kBandEdges[band + 1];

        int lowBin = std::max(1, static_cast<int>(lowFreq / binWidth));
        int highBin = std::min(specSize - 1, static_cast<int>(highFreq / binWidth));

        float sumSq = 0.0f;
        int count = 0;

        for (int bin = lowBin; bin <= highBin; ++bin)
        {
            float mag = magnitudeSpectrum[static_cast<size_t>(bin)];
            sumSq += mag * mag;
            count++;
        }

        bandRMS[static_cast<size_t>(band)] = count > 0 ? std::sqrt(sumSq / static_cast<float>(count)) : 0.0f;
    }

    // Normalize bands to 0-1 range (use a reasonable reference level)
    float maxBand = *std::max_element(bandRMS.begin(), bandRMS.end());
    if (maxBand > 0.0001f)
    {
        for (auto& b : bandRMS)
            b = std::min(1.0f, b / maxBand);
    }
}

void AudioAnalyzer::updateEnvelopes()
{
    // Approximate envelope follower using per-frame smoothing
    // Frame rate is roughly sampleRate / kFFTSize
    float frameRate = static_cast<float>(sampleRate) / static_cast<float>(kFFTSize);
    float attackCoeff = 1.0f - std::exp(-1.0f / (envelopeAttack * frameRate));
    float releaseCoeff = 1.0f - std::exp(-1.0f / (envelopeRelease * frameRate));

    for (int i = 0; i < kNumBands; ++i)
    {
        float target = bandRMS[static_cast<size_t>(i)];
        float& env = bandEnvelope[static_cast<size_t>(i)];

        if (target > env)
            env += attackCoeff * (target - env);
        else
            env += releaseCoeff * (target - env);
    }
}

void AudioAnalyzer::detectOnset()
{
    // Spectral flux onset detection
    float flux = 0.0f;
    const int specSize = kFFTSize / 2 + 1;

    for (int i = 0; i < specSize; ++i)
    {
        float diff = magnitudeSpectrum[static_cast<size_t>(i)] - 0.0f; // simplified: just magnitude
        if (diff > 0.0f)
            flux += diff;
    }

    // Compare against previous flux
    float delta = flux - prevSpectralFlux;
    onsetDetected = delta > onsetThreshold;
    prevSpectralFlux = flux;
}
