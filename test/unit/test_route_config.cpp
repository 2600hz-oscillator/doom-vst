#include "routing/RouteConfig.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("RouteConfig: parses scene name, inputs, and routes", "[route_config]")
{
    RouteConfig cfg;
    const std::string yaml = R"YAML(
scene: kill_room

inputs:
  kick:
    source: audio
    mode: band_rms
    band: [20, 200]
    smoothing: 0.05
    gain: 1.5
  velocity:
    source: midi
    mode: note_velocity
    channel: 1

routes:
  - from: kick
    to: player_speed
    scale: 2.0
  - from: velocity
    to: sector_light.all
    scale: 1.0
    blend: add
)YAML";

    REQUIRE(cfg.loadFromString(yaml));
    const auto& s = cfg.getConfig();

    REQUIRE(s.sceneName == "kill_room");
    REQUIRE(s.inputs.size() == 2);
    REQUIRE(s.routes.size() == 2);

    const auto& kick = s.inputs[0];
    REQUIRE(kick.name == "kick");
    REQUIRE(kick.source == InputChannel::Source::Audio);
    REQUIRE(kick.mode == InputChannel::Mode::BandRMS);
    REQUIRE(kick.bandLow == 20.0f);
    REQUIRE(kick.bandHigh == 200.0f);
    REQUIRE(kick.smoothing == 0.05f);
    REQUIRE(kick.gain == 1.5f);

    const auto& vel = s.inputs[1];
    REQUIRE(vel.source == InputChannel::Source::Midi);
    REQUIRE(vel.mode == InputChannel::Mode::NoteVelocity);
    REQUIRE(vel.midiChannel == 1);

    REQUIRE(s.routes[0].from == "kick");
    REQUIRE(s.routes[0].to == "player_speed");
    REQUIRE(s.routes[0].scale == 2.0f);
    REQUIRE(s.routes[0].blend == Route::Blend::Replace);

    REQUIRE(s.routes[1].blend == Route::Blend::Add);
}

TEST_CASE("RouteConfig: malformed YAML is reported, not thrown", "[route_config]")
{
    RouteConfig cfg;
    REQUIRE_FALSE(cfg.loadFromString("inputs: [unterminated"));
    REQUIRE_FALSE(cfg.getError().empty());
}

TEST_CASE("RouteConfig: empty config parses to an empty SceneConfig", "[route_config]")
{
    RouteConfig cfg;
    REQUIRE(cfg.loadFromString(""));
    REQUIRE(cfg.getConfig().sceneName.empty());
    REQUIRE(cfg.getConfig().inputs.empty());
    REQUIRE(cfg.getConfig().routes.empty());
}
