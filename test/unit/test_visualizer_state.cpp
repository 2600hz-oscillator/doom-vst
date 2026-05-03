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

TEST_CASE("VisualizerState: sampler defaults are 9 empty pads",
          "[viz_state][sampler]")
{
    patch::VisualizerState state;
    auto sm = state.getSampler();
    REQUIRE(sm.pads.size() == static_cast<size_t>(patch::kNumPads));
    for (const auto& p : sm.pads)
    {
        REQUIRE(p.name.isEmpty());
        REQUIRE(p.sampleData.empty());
        REQUIRE(p.startSample == 0);
        REQUIRE(p.endSample   == 0);
    }
}

TEST_CASE("VisualizerState: setPadMarkers updates only start/end on the named pad",
          "[viz_state][sampler]")
{
    patch::VisualizerState state;
    auto sm = patch::SamplerConfig::makeDefault();
    sm.pads[2].name             = "test.wav";
    sm.pads[2].sampleData       = std::vector<float>(1000, 0.5f);
    sm.pads[2].startSample      = 0;
    sm.pads[2].endSample        = 1000;
    state.setSampler(sm);

    state.setPadMarkers(2, 100, 800);

    auto smr = state.getSampler();
    REQUIRE(smr.pads[2].startSample        == 100);
    REQUIRE(smr.pads[2].endSample          == 800);
    REQUIRE(smr.pads[2].name               == juce::String("test.wav"));
    REQUIRE(smr.pads[2].sampleData.size()  == 1000); // unchanged

    // Out-of-range pad is a no-op, not a crash.
    state.setPadMarkers(99, 0, 0);
    state.setPadMarkers(-1, 0, 0);
}

TEST_CASE("VisualizerState: sampler XML round-trip preserves pad metadata + sample data",
          "[viz_state][sampler]")
{
    patch::VisualizerState a;
    auto sm = patch::SamplerConfig::makeDefault();

    // Pad 0: a small ramp. Round-trips through 16-bit PCM, so allow a
    // tiny quantization error (1 LSB at 16-bit ~ 3e-5).
    sm.pads[0].name             = "DSPISTOL.wav";
    sm.pads[0].sourceSampleRate = 44100.0;
    sm.pads[0].startSample      = 0;
    sm.pads[0].endSample        = 8;
    sm.pads[0].sampleData       = { 0.0f, 0.25f, 0.5f, 0.75f, -0.25f, -0.5f, -0.75f, 1.0f };

    // Pad 4: name + markers but no sample data (e.g. metadata stub).
    sm.pads[4].name        = "empty.wav";
    sm.pads[4].endSample   = 0;

    a.setSampler(sm);

    juce::String xml = a.toXmlString();
    REQUIRE_FALSE(xml.isEmpty());

    patch::VisualizerState b;
    b.fromXmlString(xml);
    auto smr = b.getSampler();

    REQUIRE(smr.pads[0].name             == juce::String("DSPISTOL.wav"));
    REQUIRE(smr.pads[0].sourceSampleRate == 44100.0);
    REQUIRE(smr.pads[0].startSample      == 0);
    REQUIRE(smr.pads[0].endSample        == 8);
    REQUIRE(smr.pads[0].sampleData.size() == 8);

    const std::vector<float> expected =
        { 0.0f, 0.25f, 0.5f, 0.75f, -0.25f, -0.5f, -0.75f, 1.0f };
    for (size_t i = 0; i < expected.size(); ++i)
        REQUIRE(std::abs(smr.pads[0].sampleData[i] - expected[i]) < 1.0e-4f);

    REQUIRE(smr.pads[4].name == juce::String("empty.wav"));
    REQUIRE(smr.pads[4].sampleData.empty());

    // Pads we never touched stay default-empty after the round-trip.
    REQUIRE(smr.pads[7].name.isEmpty());
    REQUIRE(smr.pads[7].sampleData.empty());
}
