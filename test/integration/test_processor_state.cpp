#include "PluginProcessor.h"
#include "patch/PatchSettings.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DoomVizProcessor: state save/restore round-trips patch settings",
          "[processor][state]")
{
    DoomVizProcessor a;

    auto s = patch::SpectrumSettings::makeDefault();
    s.bands[1].lowHz = 111.0f;
    s.bands[1].highHz = 222.0f;
    s.bands[1].gain01 = 0.42f;
    s.bands[1].spriteId = 92;
    s.vibe = patch::BackgroundVibe::Crtglitch;
    a.getPatchSettings().setSpectrum(s);

    juce::MemoryBlock blob;
    a.getStateInformation(blob);
    REQUIRE(blob.getSize() > 0);

    DoomVizProcessor b;
    b.setStateInformation(blob.getData(), (int) blob.getSize());
    auto restored = b.getPatchSettings().getSpectrum();

    REQUIRE(restored.bands[1].lowHz == 111.0f);
    REQUIRE(restored.bands[1].highHz == 222.0f);
    REQUIRE(restored.bands[1].gain01 == 0.42f);
    REQUIRE(restored.bands[1].spriteId == 92);
    REQUIRE(restored.vibe == patch::BackgroundVibe::Crtglitch);
}

TEST_CASE("DoomVizProcessor: setStateInformation with empty blob is safe",
          "[processor][state]")
{
    DoomVizProcessor p;
    p.setStateInformation(nullptr, 0);
    // Should fall back to defaults; no crash.
    auto s = p.getPatchSettings().getSpectrum();
    REQUIRE(s.bands.size() == patch::kSpectrumNumBands);
}

TEST_CASE("DoomVizProcessor: getStateInformation produces non-empty blob even on a fresh processor",
          "[processor][state]")
{
    DoomVizProcessor p;
    juce::MemoryBlock blob;
    p.getStateInformation(blob);
    REQUIRE(blob.getSize() > 0);
}
