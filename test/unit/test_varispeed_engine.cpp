#include "audio/VarispeedEngine.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("VarispeedEngine: ratio 1.0 reproduces the source samples + gain",
          "[sampler][varispeed]")
{
    std::vector<float> src = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    std::vector<float> outL(src.size(), 0.0f);

    VarispeedEngine eng;
    eng.prepare(48000.0, 1.0);
    int rendered = eng.render(src.data(), 0, static_cast<int>(src.size()),
                               outL.data(), nullptr,
                               static_cast<int>(outL.size()), 0.5f);

    // Last source sample is unreachable (linear interp needs p0+1 in range),
    // so we get all but the final sample.
    REQUIRE(rendered == 4);
    REQUIRE(outL[0] == 0.05f);
    REQUIRE(outL[1] == 0.10f);
    REQUIRE(outL[2] == 0.15f);
    REQUIRE(outL[3] == 0.20f);
    REQUIRE(eng.finished());
}

TEST_CASE("VarispeedEngine: stereo output writes the same sample to L and R",
          "[sampler][varispeed]")
{
    std::vector<float> src = { 0.5f, 0.5f, 0.5f };
    std::vector<float> outL(2, 0.0f);
    std::vector<float> outR(2, 0.0f);

    VarispeedEngine eng;
    eng.prepare(48000.0, 1.0);
    eng.render(src.data(), 0, 3, outL.data(), outR.data(), 2, 1.0f);

    REQUIRE(outL[0] == 0.5f);
    REQUIRE(outR[0] == 0.5f);
    REQUIRE(outL[1] == 0.5f);
    REQUIRE(outR[1] == 0.5f);
}

TEST_CASE("VarispeedEngine: ratio 2.0 advances readPos at 2x and ends mid-block",
          "[sampler][varispeed]")
{
    // 6 samples, ratio 2 → reads at positions 0, 2, 4 then runs out.
    std::vector<float> src = { 0.0f, 0.0f, 0.5f, 0.5f, 1.0f, 1.0f };
    std::vector<float> outL(8, 0.0f);

    VarispeedEngine eng;
    eng.prepare(48000.0, 2.0);
    int rendered = eng.render(src.data(), 0, 6, outL.data(), nullptr, 8, 1.0f);

    // Reads at 0.0 → 0.0, 2.0 → 0.5, 4.0 → 1.0; next would be 6.0 which is OOB.
    REQUIRE(rendered == 3);
    REQUIRE(outL[0] == 0.0f);
    REQUIRE(outL[1] == 0.5f);
    REQUIRE(outL[2] == 1.0f);
    REQUIRE(eng.finished());
}

TEST_CASE("VarispeedEngine: zero-range input is finished immediately",
          "[sampler][varispeed]")
{
    std::vector<float> src = { 0.0f };
    std::vector<float> outL(4, 0.0f);

    VarispeedEngine eng;
    eng.prepare(48000.0, 1.0);
    int rendered = eng.render(src.data(), 0, 0, outL.data(), nullptr, 4, 1.0f);
    REQUIRE(rendered == 0);
    REQUIRE(eng.finished());
}

TEST_CASE("VarispeedEngine: reset re-arms the engine for a new note",
          "[sampler][varispeed]")
{
    std::vector<float> src = { 0.0f, 0.5f, 1.0f };
    std::vector<float> outL(2, 0.0f);

    VarispeedEngine eng;
    eng.prepare(48000.0, 1.0);
    eng.render(src.data(), 0, 3, outL.data(), nullptr, 2, 1.0f);
    REQUIRE_FALSE(eng.finished());

    // Drain the engine.
    std::fill(outL.begin(), outL.end(), 0.0f);
    eng.render(src.data(), 0, 3, outL.data(), nullptr, 2, 1.0f);
    REQUIRE(eng.finished());

    // Reset + new note continues working.
    eng.prepare(48000.0, 1.0);
    REQUIRE_FALSE(eng.finished());
    std::fill(outL.begin(), outL.end(), 0.0f);
    eng.render(src.data(), 0, 3, outL.data(), nullptr, 2, 1.0f);
    REQUIRE(outL[0] == 0.0f);
    REQUIRE(outL[1] == 0.5f);
}
