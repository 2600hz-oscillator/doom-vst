#include "PluginProcessor.h"
#include "patch/VisualizerState.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DoomVizProcessor: state save/restore round-trips visualizer state",
          "[processor][state]")
{
    DoomVizProcessor a;

    auto g = patch::GlobalConfig::makeDefault();
    g.bands[1].lowHz    = 111.0f;
    g.bands[1].highHz   = 222.0f;
    g.bands[1].gain01   = 0.42f;
    g.bands[1].spriteId = 92;
    a.getVisualizerState().setGlobal(g);

    auto s = patch::SpectrumConfig::makeDefault();
    s.vibe = patch::BackgroundVibe::Crtglitch;
    a.getVisualizerState().setSpectrum(s);

    juce::MemoryBlock blob;
    a.getStateInformation(blob);
    REQUIRE(blob.getSize() > 0);

    DoomVizProcessor b;
    b.setStateInformation(blob.getData(), (int) blob.getSize());
    auto gr = b.getVisualizerState().getGlobal();
    auto sr = b.getVisualizerState().getSpectrum();

    REQUIRE(gr.bands[1].lowHz    == 111.0f);
    REQUIRE(gr.bands[1].highHz   == 222.0f);
    REQUIRE(gr.bands[1].gain01   == 0.42f);
    REQUIRE(gr.bands[1].spriteId == 92);
    REQUIRE(sr.vibe == patch::BackgroundVibe::Crtglitch);
}

TEST_CASE("DoomVizProcessor: setStateInformation with empty blob is safe",
          "[processor][state]")
{
    DoomVizProcessor p;
    p.setStateInformation(nullptr, 0);
    // Should fall back to defaults; no crash.
    auto g = p.getVisualizerState().getGlobal();
    REQUIRE(g.bands.size() == patch::kNumBands);
}

TEST_CASE("DoomVizProcessor: getStateInformation produces non-empty blob even on a fresh processor",
          "[processor][state]")
{
    DoomVizProcessor p;
    juce::MemoryBlock blob;
    p.getStateInformation(blob);
    REQUIRE(blob.getSize() > 0);
}
