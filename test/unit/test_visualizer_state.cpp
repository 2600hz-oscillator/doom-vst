#include "patch/VisualizerState.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("VisualizerState: defaults match GlobalConfig::makeDefault and SpectrumConfig::makeDefault",
          "[viz_state]")
{
    patch::VisualizerState state;
    auto g = state.getGlobal();
    auto s = state.getSpectrum();
    auto gd = patch::GlobalConfig::makeDefault();
    auto sd = patch::SpectrumConfig::makeDefault();

    REQUIRE(s.vibe == sd.vibe);
    REQUIRE(g.bands.size() == gd.bands.size());
    for (size_t i = 0; i < g.bands.size(); ++i)
    {
        REQUIRE(g.bands[i].lowHz    == gd.bands[i].lowHz);
        REQUIRE(g.bands[i].highHz   == gd.bands[i].highHz);
        REQUIRE(g.bands[i].gain01   == gd.bands[i].gain01);
        REQUIRE(g.bands[i].spriteId == gd.bands[i].spriteId);
    }
}

TEST_CASE("VisualizerState: setGlobal/setSpectrum round-trip",
          "[viz_state]")
{
    patch::VisualizerState state;
    auto g = patch::GlobalConfig::makeDefault();
    g.bands[0].gain01 = 0.123f;
    g.bands[3].spriteId = 42;
    state.setGlobal(g);

    auto s = patch::SpectrumConfig::makeDefault();
    s.vibe = patch::BackgroundVibe::Doomtex;
    state.setSpectrum(s);

    auto gr = state.getGlobal();
    auto sr = state.getSpectrum();
    REQUIRE(gr.bands[0].gain01   == 0.123f);
    REQUIRE(gr.bands[3].spriteId == 42);
    REQUIRE(sr.vibe == patch::BackgroundVibe::Doomtex);
}

TEST_CASE("VisualizerState: XML round-trip preserves global bands and spectrum config",
          "[viz_state]")
{
    patch::VisualizerState a;
    auto g = patch::GlobalConfig::makeDefault();
    g.bands[2].lowHz    = 234.0f;
    g.bands[2].highHz   = 567.0f;
    g.bands[2].gain01   = 0.42f;
    g.bands[2].spriteId = 92;
    a.setGlobal(g);

    auto s = patch::SpectrumConfig::makeDefault();
    s.vibe                  = patch::BackgroundVibe::Winamp;
    s.doomtexIndex          = 5;
    s.doomtexAutoAdvance    = false;
    s.doomtexAutoBand       = 7;
    s.doomtexAutoThreshold  = 0.73f;
    a.setSpectrum(s);

    juce::String xml = a.toXmlString();
    REQUIRE_FALSE(xml.isEmpty());

    patch::VisualizerState b;
    b.fromXmlString(xml);
    auto gr = b.getGlobal();
    auto sr = b.getSpectrum();

    REQUIRE(gr.bands[2].lowHz    == 234.0f);
    REQUIRE(gr.bands[2].highHz   == 567.0f);
    REQUIRE(gr.bands[2].gain01   == 0.42f);
    REQUIRE(gr.bands[2].spriteId == 92);
    REQUIRE(sr.vibe                 == patch::BackgroundVibe::Winamp);
    REQUIRE(sr.doomtexIndex         == 5);
    REQUIRE(sr.doomtexAutoAdvance   == false);
    REQUIRE(sr.doomtexAutoBand      == 7);
    REQUIRE(sr.doomtexAutoThreshold == 0.73f);
}

TEST_CASE("VisualizerState: SpectrumConfig defaults match the prior hard-coded behavior",
          "[viz_state]")
{
    auto s = patch::SpectrumConfig::makeDefault();
    REQUIRE(s.doomtexAutoAdvance   == true);
    REQUIRE(s.doomtexAutoBand      == 3);
    REQUIRE(s.doomtexAutoThreshold == 0.5f);
    REQUIRE(s.doomtexIndex         == 0);
}

TEST_CASE("VisualizerState: legacy DoomVizPatches XML still loads",
          "[viz_state][legacy]")
{
    // Simulates a project saved before the rename; bands lived inside <Spectrum>.
    juce::String legacy =
        "<DoomVizPatches>"
        "  <Spectrum vibe=\"4\">"
        "    <Band index=\"5\" lowHz=\"100.0\" highHz=\"500.0\" gain01=\"0.25\" spriteId=\"7\"/>"
        "  </Spectrum>"
        "</DoomVizPatches>";

    patch::VisualizerState state;
    state.fromXmlString(legacy);
    auto g = state.getGlobal();
    auto s = state.getSpectrum();

    REQUIRE(g.bands[5].lowHz    == 100.0f);
    REQUIRE(g.bands[5].highHz   == 500.0f);
    REQUIRE(g.bands[5].gain01   == 0.25f);
    REQUIRE(g.bands[5].spriteId == 7);
    REQUIRE(s.vibe == patch::BackgroundVibe::Winamp);
}

TEST_CASE("VisualizerState: malformed XML is ignored, store stays usable",
          "[viz_state]")
{
    patch::VisualizerState state;
    state.fromXmlString("<not-our-format><foo/></not-our-format>");
    auto g = state.getGlobal();
    REQUIRE(g.bands.size() == patch::kNumBands);
}
