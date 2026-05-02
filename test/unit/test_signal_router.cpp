#include "routing/SignalRouter.h"
#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace
{
    SceneConfig makeCCConfig(const std::string& dest, float scale, Route::Blend blend)
    {
        SceneConfig c;
        c.sceneName = "test";
        InputChannel cc;
        cc.name = "midi_cc1";
        cc.source = InputChannel::Source::Midi;
        cc.mode = InputChannel::Mode::CC;
        cc.ccNumber = 1;
        cc.smoothing = 0.0f;  // raw value passes through, no easing
        c.inputs.push_back(cc);

        Route r;
        r.from = "midi_cc1";
        r.to = dest;
        r.scale = scale;
        r.blend = blend;
        c.routes.push_back(r);
        return c;
    }
}

TEST_CASE("SignalRouter: returns 0 for any parameter when no config is loaded", "[signal_router]")
{
    SignalRouter router;
    AudioAnalyzer a;
    SignalBus bus;
    router.evaluate(a, bus, 0.016f);
    REQUIRE(router.getParameter("anything") == 0.0f);
}

TEST_CASE("SignalRouter: CC input is scaled and routed to a parameter", "[signal_router]")
{
    SignalRouter router;
    AudioAnalyzer a;
    SignalBus bus;

    router.loadConfig(makeCCConfig("brightness", 1.0f, Route::Blend::Replace));
    bus.setCC(1, 127);  // max
    router.evaluate(a, bus, 0.016f);

    REQUIRE_THAT(router.getParameter("brightness"), WithinAbs(1.0f, 1e-6f));
}

TEST_CASE("SignalRouter: Add blend accumulates routes onto the same destination",
          "[signal_router]")
{
    SignalRouter router;
    AudioAnalyzer a;
    SignalBus bus;

    SceneConfig cfg;
    InputChannel cc1, cc2;
    cc1.name = "a"; cc1.mode = InputChannel::Mode::CC; cc1.ccNumber = 1; cc1.smoothing = 0.0f;
    cc2.name = "b"; cc2.mode = InputChannel::Mode::CC; cc2.ccNumber = 2; cc2.smoothing = 0.0f;
    cfg.inputs = { cc1, cc2 };

    Route r1, r2;
    r1.from = "a"; r1.to = "x"; r1.scale = 0.25f; r1.blend = Route::Blend::Add;
    r2.from = "b"; r2.to = "x"; r2.scale = 0.25f; r2.blend = Route::Blend::Add;
    cfg.routes = { r1, r2 };

    router.loadConfig(cfg);
    bus.setCC(1, 127);
    bus.setCC(2, 127);
    router.evaluate(a, bus, 0.016f);

    // Each route contributes 0.25 → sum 0.5 (clamped post-route inside evaluate).
    REQUIRE_THAT(router.getParameter("x"), WithinAbs(0.5f, 1e-6f));
}

TEST_CASE("SignalRouter: Max blend keeps the strongest contributor", "[signal_router]")
{
    SignalRouter router;
    AudioAnalyzer a;
    SignalBus bus;

    SceneConfig cfg;
    InputChannel cc1, cc2;
    cc1.name = "a"; cc1.mode = InputChannel::Mode::CC; cc1.ccNumber = 1; cc1.smoothing = 0.0f;
    cc2.name = "b"; cc2.mode = InputChannel::Mode::CC; cc2.ccNumber = 2; cc2.smoothing = 0.0f;
    cfg.inputs = { cc1, cc2 };

    Route r1, r2;
    r1.from = "a"; r1.to = "x"; r1.scale = 0.5f; r1.blend = Route::Blend::Max;
    r2.from = "b"; r2.to = "x"; r2.scale = 1.0f; r2.blend = Route::Blend::Max;
    cfg.routes = { r1, r2 };

    router.loadConfig(cfg);
    bus.setCC(1, 127);  // → 0.5
    bus.setCC(2, 64);   // → ~0.504
    router.evaluate(a, bus, 0.016f);

    REQUIRE(router.getParameter("x") > 0.49f);
    REQUIRE(router.getParameter("x") <= 1.0f);
}

TEST_CASE("SignalRouter: Replace overwrites prior value", "[signal_router]")
{
    SignalRouter router;
    AudioAnalyzer a;
    SignalBus bus;

    router.loadConfig(makeCCConfig("y", 1.0f, Route::Blend::Replace));
    bus.setCC(1, 127);
    router.evaluate(a, bus, 0.016f);
    REQUIRE_THAT(router.getParameter("y"), WithinAbs(1.0f, 1e-6f));

    bus.setCC(1, 0);
    router.evaluate(a, bus, 0.016f);
    REQUIRE_THAT(router.getParameter("y"), WithinAbs(0.0f, 1e-6f));
}
