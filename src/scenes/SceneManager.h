#pragma once

#include "Scene.h"
#include <memory>
#include <vector>

class DoomEngine;

// Manages scene lifecycle and switching (via MIDI Program Change).
class SceneManager
{
public:
    SceneManager();

    // Add a scene. Order determines program change index (0, 1, 2...).
    void addScene(std::unique_ptr<Scene> scene);

    // Initialize all scenes and activate the first one.
    void init(DoomEngine& engine);

    // Switch to scene by index. Safe to call every frame (no-ops if already active).
    void switchTo(int index, DoomEngine& engine);

    // Update + render the active scene. Returns RGBA pixels.
    const uint8_t* updateAndRender(DoomEngine& engine, const ParameterMap& params,
                                    float deltaTime);

    int getActiveIndex() const { return activeIndex; }
    int getNumScenes() const { return static_cast<int>(scenes.size()); }

private:
    std::vector<std::unique_ptr<Scene>> scenes;
    int activeIndex = -1;
};
