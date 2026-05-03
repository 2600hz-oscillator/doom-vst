#include "patch/SampleSetIO.h"
#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>

namespace
{
    juce::File findRepoDefaultDvset()
    {
        // Walks up from the test binary to the project root, then
        // resources/sets/default.dvset. The CMake working dir for
        // catch_discover_tests is the build dir.
        auto here = juce::File::getCurrentWorkingDirectory();
        while (here != juce::File())
        {
            auto candidate = here.getChildFile("resources/sets/default.dvset");
            if (candidate.existsAsFile()) return candidate;
            auto parent = here.getParentDirectory();
            if (parent == here) break;
            here = parent;
        }
        return {};
    }
}

TEST_CASE("SampleSetIO: bundled default.dvset loads with all 9 pads non-empty",
          "[sampler][set_io]")
{
    auto file = findRepoDefaultDvset();
    REQUIRE(file.existsAsFile());

    auto sm = patch::SampleSetIO::loadFromFile(file, 48000.0);
    REQUIRE(sm.has_value());

    for (int i = 0; i < patch::kNumPads; ++i)
    {
        INFO("Pad " << i);
        REQUIRE(! sm->pads[i].sampleData.empty());
        REQUIRE(! sm->pads[i].name.isEmpty());
        REQUIRE(sm->pads[i].endSample > 0);
    }
}

TEST_CASE("SampleSetIO: round-trip preserves a 9-pad config",
          "[sampler][set_io]")
{
    patch::SamplerConfig sm;
    for (int i = 0; i < patch::kNumPads; ++i)
    {
        sm.pads[i].name             = "pad_" + juce::String(i) + ".wav";
        sm.pads[i].sourceSampleRate = 48000.0;
        sm.pads[i].startSample      = 0;
        sm.pads[i].endSample        = 16;
        sm.pads[i].sampleData       = std::vector<float>(16, 0.5f);
    }

    auto temp = juce::File::createTempFile("dvset");
    REQUIRE(patch::SampleSetIO::saveToFile(sm, temp));

    auto loaded = patch::SampleSetIO::loadFromFile(temp, 48000.0);
    REQUIRE(loaded.has_value());

    for (int i = 0; i < patch::kNumPads; ++i)
    {
        REQUIRE(loaded->pads[i].name             == sm.pads[i].name);
        REQUIRE(loaded->pads[i].sampleData.size() == sm.pads[i].sampleData.size());
    }

    temp.deleteFile();
}
