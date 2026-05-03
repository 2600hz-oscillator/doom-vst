#pragma once

// Pluggable per-voice pitch processor. 1.0 ships VarispeedEngine; 1.1 will
// add a TimeStretchEngine implementation that the SamplerEngine can swap
// in per voice without touching call sites.
class PitchEngine
{
public:
    virtual ~PitchEngine() = default;

    // Configure for a new note. Called once before render() loop.
    virtual void prepare(double hostSampleRate, double pitchRatio) = 0;

    // Add resampled audio into outputL (and outputR if non-null). Returns
    // frames actually rendered; < numSamples means the underlying sample
    // ended mid-block, after which finished() returns true.
    virtual int render(const float* sampleData,
                       int sampleStart, int sampleEnd,
                       float* outputL, float* outputR,
                       int numSamples, float gain) = 0;

    virtual bool finished() const = 0;

    virtual void reset() = 0;
};
