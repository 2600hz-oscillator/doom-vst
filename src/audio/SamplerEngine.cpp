#include "SamplerEngine.h"
#include "patch/VisualizerState.h"

#include <algorithm>
#include <cmath>

SamplerEngine::SamplerEngine(const patch::VisualizerState& state)
    : vizState(state)
{
}

void SamplerEngine::prepare(double hostSampleRate, int samplesPerBlock)
{
    sampleRate = hostSampleRate;
    voiceScratch.assign(static_cast<size_t>(std::max(1, samplesPerBlock)), 0.0f);
    for (auto& pad : voices)
        for (auto& v : pad)
            v = {};
}

void SamplerEngine::processMidi(const juce::MidiBuffer& midi)
{
    // Pull the latest sampler state once per block. Voices already in
    // flight will pick up the new sample buffer the next time they
    // render.
    liveConfig = vizState.getSampler();

    // Drain any GUI-requested previews. Atomic exchange is fence enough
    // to see the GUI thread's pendingPreview bit + new sampleData.
    const juce::uint32 pending = pendingPreview.exchange(0, std::memory_order_acquire);
    for (int i = 0; i < patch::kNumPads; ++i)
        if (pending & (1u << i))
            noteOn(i, 60, 100.0f / 127.0f);

    for (const auto metadata : midi)
    {
        const auto msg = metadata.getMessage();
        if (! msg.isNoteOn()) continue;
        const int channel = msg.getChannel(); // 1..16
        if (channel < 1 || channel > patch::kNumPads) continue;
        noteOn(channel - 1, msg.getNoteNumber(), msg.getFloatVelocity());
    }
}

void SamplerEngine::requestPreview(int padIndex)
{
    if (padIndex < 0 || padIndex >= patch::kNumPads) return;
    pendingPreview.fetch_or(1u << padIndex, std::memory_order_release);
}

void SamplerEngine::noteOn(int padIndex, int midiNote, float velocity01)
{
    if (padIndex < 0 || padIndex >= patch::kNumPads) return;

    const auto& pad = liveConfig.pads[padIndex];
    if (pad.sampleData.empty()) return;

    const int dataSize = static_cast<int>(pad.sampleData.size());
    const int start = juce::jlimit(0, dataSize, pad.startSample);
    const int end   = juce::jlimit(start, dataSize,
                                    pad.endSample > 0 ? pad.endSample : dataSize);
    if (end - start < 2) return;

    auto& slot = voices[padIndex];

    // Find an inactive voice; otherwise steal the oldest active one.
    int chosen = 0;
    bool foundFree = false;
    int oldestAge = -1;
    for (int i = 0; i < kVoicesPerPad; ++i)
    {
        if (! slot[i].active) { chosen = i; foundFree = true; break; }
        if (slot[i].ageInBlocks > oldestAge)
        {
            oldestAge = slot[i].ageInBlocks;
            chosen    = i;
        }
    }
    juce::ignoreUnused(foundFree);

    Voice& v = slot[chosen];
    v.active      = true;
    v.midiNote    = midiNote;
    v.gain        = juce::jlimit(0.0f, 1.0f, velocity01);
    v.ageInBlocks = 0;
    v.sampleStart = start;
    v.sampleEnd   = end;

    const double ratio = std::pow(2.0, (midiNote - 60) / 12.0);
    v.engine.prepare(sampleRate, ratio);
}

void SamplerEngine::render(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (numSamples <= 0) return;
    if (static_cast<int>(voiceScratch.size()) < numSamples)
        return; // prepare() sets this; defensive no-op.

    const int numChans = buffer.getNumChannels();
    if (numChans == 0) return;

    float* L = buffer.getWritePointer(0);
    float* R = numChans > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int padIdx = 0; padIdx < patch::kNumPads; ++padIdx)
    {
        const auto& pad = liveConfig.pads[padIdx];
        const int   dataSize = static_cast<int>(pad.sampleData.size());
        const float* data    = dataSize > 0 ? pad.sampleData.data() : nullptr;

        for (auto& v : voices[padIdx])
        {
            if (! v.active) continue;

            // Defensive clamp in case the pad's data shrank between blocks.
            const int clampedEnd = juce::jmin(v.sampleEnd, dataSize);
            if (data == nullptr || clampedEnd - v.sampleStart < 2)
            {
                v.active = false;
                v.engine.reset();
                continue;
            }

            const int rendered = v.engine.render(data, v.sampleStart, clampedEnd,
                                                  L, R, numSamples, v.gain);
            juce::ignoreUnused(rendered);

            ++v.ageInBlocks;
            if (v.engine.finished())
            {
                v.active = false;
                v.engine.reset();
            }
        }
    }
}

int SamplerEngine::activeVoiceCount(int padIndex) const
{
    if (padIndex < 0 || padIndex >= patch::kNumPads) return 0;
    int n = 0;
    for (const auto& v : voices[padIndex])
        if (v.active) ++n;
    return n;
}
