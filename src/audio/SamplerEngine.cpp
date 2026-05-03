#include "SamplerEngine.h"
#include "patch/VisualizerState.h"

SamplerEngine::SamplerEngine(const patch::VisualizerState& state)
    : vizState(state)
{
}

void SamplerEngine::prepare(double hostSampleRate)
{
    sampleRate = hostSampleRate;
}

void SamplerEngine::render(juce::AudioBuffer<float>&, int)
{
    // Phase-2 stub: no voices yet. Phase-3 implements voice rendering.
    juce::ignoreUnused(vizState);
}
