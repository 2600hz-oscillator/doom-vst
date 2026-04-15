#pragma once

#include "Scene.h"
#include <vector>
#include <cstdint>

// Scene A: Auto-navigating kill room in E1M1 outdoor area.
// Audio drives lighting, onsets spawn monsters, camera moves on beat.
class KillRoomScene : public Scene
{
public:
    KillRoomScene();

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "kill_room"; }
    void cleanup(DoomEngine& engine) override;

private:
    // Camera state
    int32_t camX = 0, camY = 0, camZ = 0;
    uint32_t camAngle = 0;
    float moveSpeed = 0.0f;
    float turnTimer = 0.0f;
    float turnDirection = 1.0f;
    float blockedTimer = 0.0f;  // when >0, we're turning to escape a wall

    // Monster tracking
    struct SpawnedMonster { int handle; float lifetime; };
    std::vector<SpawnedMonster> monsters;
    float spawnCooldown = 0.0f;
    static constexpr int kMaxMonsters = 50;
    static constexpr float kMonsterLifetime = 15.0f;
    static constexpr float kSpawnCooldown = 0.3f;

    // Outdoor sector IDs (E1M1 sky sectors)
    std::vector<int> outdoorSectors;
    std::vector<int> baseLightLevels;

    // Monster types available in shareware (mobjtype_t enum values)
    static constexpr int kMonsterTypes[] = {
        11,  // MT_TROOP (Imp)
        1,   // MT_POSSESSED (Zombieman)
        2,   // MT_SHOTGUY (Shotgun Guy)
    };
    static constexpr int kNumMonsterTypes = 3;

    void findOutdoorSectors(DoomEngine& engine);
    void updateCamera(DoomEngine& engine, const ParameterMap& params, float deltaTime);
    void updateMonsters(DoomEngine& engine, const ParameterMap& params, float deltaTime);
    void updateLighting(DoomEngine& engine, const ParameterMap& params);
    void updateCombat(DoomEngine& engine, const ParameterMap& params, float deltaTime);

    float combatCooldown = 0.0f;
};
