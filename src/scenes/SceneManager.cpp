#include "SceneManager.h"
#include "doom/DoomEngine.h"

SceneManager::SceneManager() = default;

void SceneManager::addScene(std::unique_ptr<Scene> scene)
{
    scenes.push_back(std::move(scene));
}

void SceneManager::init(DoomEngine& engine)
{
    if (!scenes.empty())
    {
        activeIndex = 0;
        scenes[0]->init(engine);
    }
}

void SceneManager::switchTo(int index, DoomEngine& engine)
{
    if (index < 0 || index >= static_cast<int>(scenes.size()))
        return;
    if (index == activeIndex)
        return;

    if (activeIndex >= 0)
        scenes[static_cast<size_t>(activeIndex)]->cleanup(engine);

    // Reload the map to fully reset all engine state (monsters, textures, sectors)
    engine.loadMap(1, 1);

    activeIndex = index;
    scenes[static_cast<size_t>(activeIndex)]->init(engine);
}

const uint8_t* SceneManager::updateAndRender(DoomEngine& engine,
                                              const ParameterMap& params,
                                              float deltaTime)
{
    if (activeIndex < 0 || activeIndex >= static_cast<int>(scenes.size()))
        return nullptr;

    auto& scene = scenes[static_cast<size_t>(activeIndex)];
    scene->update(engine, params, deltaTime);
    return scene->render(engine);
}
