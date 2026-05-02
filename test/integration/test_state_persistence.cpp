#include "PluginProcessor.h"
#include "patch/VisualizerState.h"
#include <catch2/catch_test_macros.hpp>

// Sanity-check that VisualizerState configuration survives the kinds of
// scene-switch and lifecycle events that happen during live performance.
// Catches regressions where someone "fixes" a scene's init() by clobbering
// fields that should persist.

namespace
{
    constexpr double kSampleRate = 44100.0;
    constexpr int    kBlockSize  = 512;

    void prepare(DoomVizProcessor& p)
    {
        p.setPlayConfigDetails(2, 2, kSampleRate, kBlockSize);
        p.prepareToPlay(kSampleRate, kBlockSize);
    }

    patch::GlobalConfig makeNonDefaultGlobal()
    {
        auto g = patch::GlobalConfig::makeDefault();
        g.bands[0].lowHz    = 25.0f;
        g.bands[0].highHz   = 90.0f;
        g.bands[0].gain01   = 0.4f;
        g.bands[0].spriteId = 39;       // SARG (Demon)
        g.bands[3].gain01   = 0.85f;
        g.bands[3].spriteId = 40;       // HEAD (Cacodemon)
        return g;
    }

    patch::SpectrumConfig makeNonDefaultSpectrum()
    {
        auto s = patch::SpectrumConfig::makeDefault();
        s.vibe                 = patch::BackgroundVibe::Crtglitch;
        s.doomtexIndex         = 5;
        s.doomtexAutoAdvance   = false;
        s.doomtexAutoBand      = 7;
        s.doomtexAutoThreshold = 0.31f;
        return s;
    }
}

TEST_CASE("VisualizerState: config survives MIDI Program Change scene cycles",
          "[processor][state][persistence]")
{
    DoomVizProcessor p;
    prepare(p);

    p.getVisualizerState().setGlobal(makeNonDefaultGlobal());
    p.getVisualizerState().setSpectrum(makeNonDefaultSpectrum());

    // Cycle through scenes 0 → 1 → 2 → 0 via MIDI Program Change. The
    // processor's processBlock parses PCs into SignalBus; the renderer
    // (not exercised here) is what reads sceneOverride / currentSceneIndex
    // and triggers init(). What we're testing is that nothing in the
    // *processor* path resets state on a PC.
    juce::AudioBuffer<float> audio(2, kBlockSize);
    audio.clear();

    for (int scene : { 0, 1, 2, 0, 1 })
    {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::programChange(1, scene), 0);
        p.processBlock(audio, midi);
    }

    auto g = p.getVisualizerState().getGlobal();
    auto s = p.getVisualizerState().getSpectrum();

    REQUIRE(g.bands[0].lowHz    == 25.0f);
    REQUIRE(g.bands[0].highHz   == 90.0f);
    REQUIRE(g.bands[0].gain01   == 0.4f);
    REQUIRE(g.bands[0].spriteId == 39);
    REQUIRE(g.bands[3].gain01   == 0.85f);
    REQUIRE(g.bands[3].spriteId == 40);

    REQUIRE(s.vibe                 == patch::BackgroundVibe::Crtglitch);
    REQUIRE(s.doomtexIndex         == 5);
    REQUIRE(s.doomtexAutoAdvance   == false);
    REQUIRE(s.doomtexAutoBand      == 7);
    REQUIRE(s.doomtexAutoThreshold == 0.31f);
}

TEST_CASE("VisualizerState: prepareToPlay does not reset visualizer state",
          "[processor][state][persistence]")
{
    DoomVizProcessor p;

    // Set state *before* the host calls prepareToPlay — this matches the
    // setStateInformation → prepareToPlay order a host uses on project load.
    p.getVisualizerState().setGlobal(makeNonDefaultGlobal());
    p.getVisualizerState().setSpectrum(makeNonDefaultSpectrum());

    prepare(p);

    auto g = p.getVisualizerState().getGlobal();
    auto s = p.getVisualizerState().getSpectrum();
    REQUIRE(g.bands[0].spriteId    == 39);
    REQUIRE(s.doomtexIndex         == 5);
    REQUIRE(s.doomtexAutoThreshold == 0.31f);
}
