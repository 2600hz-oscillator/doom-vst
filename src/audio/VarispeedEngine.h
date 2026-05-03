#pragma once

#include "PitchEngine.h"

// Linear-interpolated read-pointer pitch engine. Pitch ratio > 1 plays
// faster + higher; ratio < 1 plays slower + lower (SP-404 character).
class VarispeedEngine : public PitchEngine
{
public:
    void prepare(double hostSampleRate, double pitchRatio) override;
    int  render(const float* sampleData,
                int sampleStart, int sampleEnd,
                float* outputL, float* outputR,
                int numSamples, float gain) override;
    bool finished() const override { return finishedFlag; }
    void reset() override;

private:
    double readPos     { 0.0 };
    double pitchRatio  { 1.0 };
    bool   finishedFlag{ false };
};
