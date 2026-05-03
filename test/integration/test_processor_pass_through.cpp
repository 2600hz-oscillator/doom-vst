#include "PluginProcessor.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <cmath>

namespace
{
    // POSIX exposes M_PI through <cmath>; MSVC requires _USE_MATH_DEFINES
    // before <math.h>. Use a portable local constant.
    constexpr double kPi = 3.14159265358979323846;

    constexpr double kSampleRate = 44100.0;
    constexpr int    kBlockSize  = 512;

    void prepare(DoomVizProcessor& p)
    {
        p.setPlayConfigDetails(2, 2, kSampleRate, kBlockSize);
        p.prepareToPlay(kSampleRate, kBlockSize);
    }

    void fillSine(juce::AudioBuffer<float>& buf, float freqHz, float amp = 0.5f)
    {
        const double w = 2.0 * kPi * freqHz / kSampleRate;
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            float* p = buf.getWritePointer(ch);
            for (int i = 0; i < buf.getNumSamples(); ++i)
                p[i] = amp * static_cast<float>(std::sin(w * i + ch * 0.5));
        }
    }
}

TEST_CASE("DoomVizProcessor: processBlock leaves audio bytes untouched", "[processor]")
{
    DoomVizProcessor p;
    prepare(p);

    juce::AudioBuffer<float> in(2, kBlockSize);
    fillSine(in, 440.0f);

    // Snapshot before processing
    std::vector<float> snapshotL(in.getReadPointer(0), in.getReadPointer(0) + kBlockSize);
    std::vector<float> snapshotR(in.getReadPointer(1), in.getReadPointer(1) + kBlockSize);

    juce::MidiBuffer midi;  // empty
    p.processBlock(in, midi);

    for (int i = 0; i < kBlockSize; ++i)
    {
        REQUIRE(in.getReadPointer(0)[i] == snapshotL[i]);
        REQUIRE(in.getReadPointer(1)[i] == snapshotR[i]);
    }
}

TEST_CASE("DoomVizProcessor: processBlock pushes mono-downmixed samples into the SignalBus",
          "[processor]")
{
    DoomVizProcessor p;
    prepare(p);

    juce::AudioBuffer<float> in(2, kBlockSize);
    fillSine(in, 1000.0f, 0.4f);

    juce::MidiBuffer midi;
    p.processBlock(in, midi);

    std::vector<float> pulled(kBlockSize, 0.0f);
    int n = p.getSignalBus().pullAudioSamples(pulled.data(), kBlockSize);

    REQUIRE(n == kBlockSize);

    // At least some pulled samples should be non-zero (we fed a real sine).
    int nonZero = 0;
    for (float s : pulled) if (std::abs(s) > 1e-4f) ++nonZero;
    REQUIRE(nonZero > kBlockSize / 4);  // sine has many non-zero samples
}

TEST_CASE("DoomVizProcessor: silent input pushes silence into the SignalBus",
          "[processor]")
{
    DoomVizProcessor p;
    prepare(p);

    juce::AudioBuffer<float> in(2, kBlockSize);
    in.clear();

    juce::MidiBuffer midi;
    p.processBlock(in, midi);

    std::vector<float> pulled(kBlockSize, 0.0f);
    p.getSignalBus().pullAudioSamples(pulled.data(), kBlockSize);
    for (float s : pulled) REQUIRE(s == 0.0f);
}

TEST_CASE("DoomVizProcessor: multiple instances construct/destruct cleanly",
          "[processor]")
{
    // Catches static-lifetime / globals issues that show up when a host
    // instantiates many copies of the plugin (per-track + presets).
    for (int i = 0; i < 5; ++i)
    {
        DoomVizProcessor p;
        p.setPlayConfigDetails(2, 2, kSampleRate, kBlockSize);
        p.prepareToPlay(kSampleRate, kBlockSize);
        p.releaseResources();
    }
    SUCCEED();
}
