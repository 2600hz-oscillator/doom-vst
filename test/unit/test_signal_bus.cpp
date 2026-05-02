#include "audio/SignalBus.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("SignalBus: ring buffer round-trips audio samples", "[signal_bus]")
{
    SignalBus bus(256);

    std::vector<float> in { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };
    bus.pushAudioSamples(in.data(), (int) in.size());

    std::vector<float> out(in.size(), 0.0f);
    int n = bus.pullAudioSamples(out.data(), (int) out.size());

    REQUIRE(n == (int) in.size());
    for (size_t i = 0; i < in.size(); ++i)
        REQUIRE(out[i] == in[i]);
}

TEST_CASE("SignalBus: pull on empty buffer returns 0", "[signal_bus]")
{
    SignalBus bus(64);
    float buf[8] = {};
    REQUIRE(bus.pullAudioSamples(buf, 8) == 0);
}

TEST_CASE("SignalBus: ring buffer drops samples when overrun", "[signal_bus]")
{
    SignalBus bus(64);
    std::vector<float> big(200, 1.0f);
    bus.pushAudioSamples(big.data(), (int) big.size());
    // Should have written at most 64 samples and silently dropped the rest.
    std::vector<float> out(256, 0.0f);
    int n = bus.pullAudioSamples(out.data(), (int) out.size());
    REQUIRE(n <= 64);
    REQUIRE(n > 0);
}

TEST_CASE("SignalBus: MIDI state read/write", "[signal_bus]")
{
    SignalBus bus;
    REQUIRE(bus.getLastNote() == -1);
    REQUIRE(bus.getLastVelocity() == 0);

    bus.setNoteOn(60, 100);
    REQUIRE(bus.getLastNote() == 60);
    REQUIRE(bus.getLastVelocity() == 100);
    REQUIRE(bus.isNoteHeld(60));
    REQUIRE_FALSE(bus.isNoteHeld(61));

    bus.setNoteOff(60);
    REQUIRE_FALSE(bus.isNoteHeld(60));

    bus.setCC(7, 64);
    REQUIRE(bus.getCC(7) == 64);
    REQUIRE(bus.getCC(0) == 0);

    bus.setProgramChange(2);
    REQUIRE(bus.getProgramChange() == 2);

    REQUIRE(bus.getMidiClockCount() == 0);
    bus.incrementMidiClock();
    bus.incrementMidiClock();
    REQUIRE(bus.getMidiClockCount() == 2);
}

TEST_CASE("SignalBus: out-of-range CC and note queries are safe", "[signal_bus]")
{
    SignalBus bus;
    REQUIRE(bus.getCC(-1) == 0);
    REQUIRE(bus.getCC(128) == 0);
    REQUIRE_FALSE(bus.isNoteHeld(-1));
    REQUIRE_FALSE(bus.isNoteHeld(128));
}
