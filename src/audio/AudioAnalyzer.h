#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

// Analyzes audio pulled from the SignalBus on the render thread.
// Provides FFT magnitudes, band RMS, envelope following, and onset detection.
class AudioAnalyzer
{
public:
    static constexpr int kFFTOrder = 11;            // 2048 samples
    static constexpr int kFFTSize = 1 << kFFTOrder; // 2048
    static constexpr int kNumBands = 16;            // Log-spaced frequency bands

    AudioAnalyzer();

    // Call from render thread with new audio samples (mono).
    // Accumulates into FFT window; processes when enough samples arrive.
    void pushSamples(const float* data, int numSamples);

    // Set the sample rate for frequency calculations.
    void setSampleRate(double sr) { sampleRate = sr; }
    double getSampleRate() const { return sampleRate; }

    // --- Results (read from render thread after pushSamples) ---

    // Raw FFT magnitude spectrum (kFFTSize/2 + 1 bins)
    const float* getMagnitudeSpectrum() const { return magnitudeSpectrum.data(); }
    int getSpectrumSize() const { return kFFTSize / 2 + 1; }

    // Band RMS values (kNumBands log-spaced bands, 0.0-1.0 normalized)
    const float* getBandRMS() const { return bandRMS.data(); }
    int getNumBands() const { return kNumBands; }

    // Smoothed band values (envelope follower with attack/release)
    const float* getBandEnvelope() const { return bandEnvelope.data(); }

    // Overall RMS level (0.0-1.0)
    float getRMSLevel() const { return rmsLevel; }

    // Onset detected this frame (true = transient detected)
    bool getOnset() const { return onsetDetected; }

    // Configuration
    void setEnvelopeAttack(float seconds) { envelopeAttack = seconds; }
    void setEnvelopeRelease(float seconds) { envelopeRelease = seconds; }
    void setOnsetThreshold(float threshold) { onsetThreshold = threshold; }

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    double sampleRate = 44100.0;

    // FFT accumulation buffer
    std::array<float, kFFTSize> fftInput {};
    int fftInputPos = 0;
    bool fftReady = false;

    // FFT output
    std::array<float, kFFTSize * 2> fftData {};
    std::vector<float> magnitudeSpectrum;

    // Band analysis
    std::array<float, kNumBands> bandRMS {};
    std::array<float, kNumBands> bandEnvelope {};

    // Envelope follower params
    float envelopeAttack = 0.01f;   // seconds
    float envelopeRelease = 0.15f;  // seconds

    // Onset detection
    float onsetThreshold = 0.3f;
    float prevSpectralFlux = 0.0f;
    bool onsetDetected = false;

    // Overall level
    float rmsLevel = 0.0f;

    void processFFT();
    void computeBands();
    void updateEnvelopes();
    void detectOnset();

    // Band edge frequencies (log-spaced from 20Hz to 20kHz)
    static constexpr float kBandEdges[kNumBands + 1] = {
        20, 40, 80, 160, 315, 630, 1250, 2500,
        5000, 8000, 10000, 12500, 14000, 16000, 18000, 19000, 20000
    };
};
