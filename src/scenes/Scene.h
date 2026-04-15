#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

class DoomEngine;
class SignalRouter;

using ParameterMap = std::unordered_map<std::string, float>;

// Abstract scene interface. Each scene drives the Doom renderer differently
// based on routed audio/MIDI parameters.
class Scene
{
public:
    virtual ~Scene() = default;

    // Called once when the scene becomes active.
    virtual void init(DoomEngine& engine) = 0;

    // Called each frame with the current parameter values and delta time.
    virtual void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) = 0;

    // Called each frame to render. For 3D scenes, just call engine.renderFrame().
    // For 2D scenes (Scene B), render to the provided RGBA buffer directly.
    // Returns pointer to 320x200 RGBA pixel data.
    virtual const uint8_t* render(DoomEngine& engine) = 0;

    // Scene name for identification.
    virtual std::string getName() const = 0;

    // Called when the scene is deactivated.
    virtual void cleanup(DoomEngine& engine) = 0;
};
