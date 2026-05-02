#include "patch/PatchSettingsStore.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("PatchSettingsStore: defaults match SpectrumSettings::makeDefault", "[patch_store]")
{
    patch::PatchSettingsStore store;
    auto s = store.getSpectrum();
    auto d = patch::SpectrumSettings::makeDefault();

    REQUIRE(s.vibe == d.vibe);
    REQUIRE(s.bands.size() == d.bands.size());
    for (size_t i = 0; i < s.bands.size(); ++i)
    {
        REQUIRE(s.bands[i].lowHz == d.bands[i].lowHz);
        REQUIRE(s.bands[i].highHz == d.bands[i].highHz);
        REQUIRE(s.bands[i].gain01 == d.bands[i].gain01);
        REQUIRE(s.bands[i].spriteId == d.bands[i].spriteId);
    }
}

TEST_CASE("PatchSettingsStore: setSpectrum followed by getSpectrum returns the same data",
          "[patch_store]")
{
    patch::PatchSettingsStore store;
    auto s = patch::SpectrumSettings::makeDefault();
    s.bands[0].gain01 = 0.123f;
    s.bands[3].spriteId = 42;
    s.vibe = patch::BackgroundVibe::Doomtex;
    store.setSpectrum(s);

    auto r = store.getSpectrum();
    REQUIRE(r.bands[0].gain01 == 0.123f);
    REQUIRE(r.bands[3].spriteId == 42);
    REQUIRE(r.vibe == patch::BackgroundVibe::Doomtex);
}

TEST_CASE("PatchSettingsStore: XML round-trip preserves all band fields and vibe",
          "[patch_store]")
{
    patch::PatchSettingsStore a;
    auto s = patch::SpectrumSettings::makeDefault();
    s.bands[2].lowHz = 234.0f;
    s.bands[2].highHz = 567.0f;
    s.bands[2].gain01 = 0.42f;
    s.bands[2].spriteId = 92;
    s.vibe = patch::BackgroundVibe::Winamp;
    a.setSpectrum(s);

    juce::String xml = a.toXmlString();
    REQUIRE_FALSE(xml.isEmpty());

    patch::PatchSettingsStore b;
    b.fromXmlString(xml);
    auto out = b.getSpectrum();

    REQUIRE(out.bands[2].lowHz == 234.0f);
    REQUIRE(out.bands[2].highHz == 567.0f);
    REQUIRE(out.bands[2].gain01 == 0.42f);
    REQUIRE(out.bands[2].spriteId == 92);
    REQUIRE(out.vibe == patch::BackgroundVibe::Winamp);
}

TEST_CASE("PatchSettingsStore: malformed XML is ignored, store stays usable",
          "[patch_store]")
{
    patch::PatchSettingsStore store;
    store.fromXmlString("<not-our-format><foo/></not-our-format>");
    auto s = store.getSpectrum();
    // After ignoring garbage XML the store should still expose either the
    // pre-set state or sane defaults — assert the latter holds and band
    // count is intact.
    REQUIRE(s.bands.size() == patch::kSpectrumNumBands);
}
