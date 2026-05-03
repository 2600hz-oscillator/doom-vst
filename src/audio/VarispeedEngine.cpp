#include "VarispeedEngine.h"

void VarispeedEngine::prepare(double, double ratio)
{
    pitchRatio   = ratio > 0.0 ? ratio : 1.0;
    readPos      = 0.0;
    finishedFlag = false;
}

void VarispeedEngine::reset()
{
    readPos      = 0.0;
    finishedFlag = false;
}

int VarispeedEngine::render(const float* sampleData,
                            int sampleStart, int sampleEnd,
                            float* outputL, float* outputR,
                            int numSamples, float gain)
{
    if (finishedFlag || sampleData == nullptr || outputL == nullptr)
    {
        finishedFlag = true;
        return 0;
    }

    const int range = sampleEnd - sampleStart;
    if (range < 2)
    {
        finishedFlag = true;
        return 0;
    }

    int rendered = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        const int p0 = static_cast<int>(readPos);
        if (p0 + 1 >= range)
        {
            finishedFlag = true;
            break;
        }
        const float frac = static_cast<float>(readPos - p0);
        const float s = sampleData[sampleStart + p0]     * (1.0f - frac)
                      + sampleData[sampleStart + p0 + 1] * frac;
        const float scaled = s * gain;
        outputL[i] += scaled;
        if (outputR != nullptr) outputR[i] += scaled;

        readPos += pitchRatio;
        ++rendered;
    }
    return rendered;
}
