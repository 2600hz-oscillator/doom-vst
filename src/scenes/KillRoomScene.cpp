#include "KillRoomScene.h"
#include "doom/DoomEngine.h"
#include "doom_renderer.h"
#include <cmath>
#include <algorithm>

KillRoomScene::KillRoomScene() = default;

void KillRoomScene::init(DoomEngine& engine)
{
    engine.getPlayerPos(camX, camY, camZ, camAngle);
    moveSpeed = 0.0f;
    turnTimer = 0.0f;
    blockedTimer = 0.0f;
    turnDirection = 1.0f;
    spawnCooldown = 0.0f;
    combatCooldown = 0.0f;
    monsters.clear();

    engine.setGodMode(true);
    engine.giveWeapon(2); // wp_shotgun

    findOutdoorSectors(engine);
}

void KillRoomScene::findOutdoorSectors(DoomEngine& engine)
{
    outdoorSectors.clear();
    baseLightLevels.clear();

    doom_map_info_t info = doom_get_map_info();
    for (int i = 0; i < info.num_sectors; ++i)
    {
        if (doom_sector_is_outdoor(i))
        {
            outdoorSectors.push_back(i);
            baseLightLevels.push_back(engine.getSectorLight(i));
        }
    }
}

void KillRoomScene::update(DoomEngine& engine, const ParameterMap& params,
                            float deltaTime)
{
    // Respawn if somehow dead (shouldn't happen with god mode, but safety net)
    if (engine.isPlayerDead())
    {
        engine.respawnPlayer();
        engine.getPlayerPos(camX, camY, camZ, camAngle);
        engine.setGodMode(true);
    }

    updateCombat(engine, params, deltaTime);
    updateCamera(engine, params, deltaTime);
    updateMonsters(engine, params, deltaTime);
    updateLighting(engine, params);

    engine.tick();
}

static float getParam(const ParameterMap& params, const std::string& key, float fallback)
{
    auto it = params.find(key);
    return (it != params.end()) ? it->second : fallback;
}

void KillRoomScene::updateCamera(DoomEngine& engine, const ParameterMap& params, float deltaTime)
{
    float speedParam = getParam(params, "player_speed", 0.0f);
    float baseSpeed = 400.0f;
    float audioBoost = speedParam * 600.0f;
    moveSpeed = (baseSpeed + audioBoost) * 65536.0f;

    // Periodic gentle turn for variety
    turnTimer += deltaTime;
    if (turnTimer > 5.0f)
    {
        turnTimer = 0.0f;
        turnDirection = -turnDirection;
    }

    // If we're blocked, spend time turning in place before trying to move again
    if (blockedTimer > 0.0f)
    {
        blockedTimer -= deltaTime;
        camAngle += static_cast<uint32_t>(turnDirection * 500000000.0f * deltaTime);
        engine.setCameraAngle(camAngle);
        return;
    }

    // Gentle ambient turn + audio shake
    float shake = getParam(params, "camera_shake", 0.0f);
    float gentleTurn = turnDirection * 0.15f
                       + shake * 2.0f * (std::fmod(turnTimer * 17.0f, 2.0f) - 1.0f);
    camAngle += static_cast<uint32_t>(gentleTurn * 100000000.0f * deltaTime);
    engine.setCameraAngle(camAngle);

    // Try to move forward
    float angleRad = static_cast<float>(camAngle) * (2.0f * 3.14159265f / 4294967296.0f);
    int32_t dx = static_cast<int32_t>(std::cos(angleRad) * moveSpeed * deltaTime);
    int32_t dy = static_cast<int32_t>(std::sin(angleRad) * moveSpeed * deltaTime);

    if (engine.movePlayer(dx, dy))
    {
        engine.getPlayerPos(camX, camY, camZ, camAngle);
    }
    else
    {
        blockedTimer = 0.4f + 0.3f * (std::fmod(turnTimer * 7.0f, 1.0f));
        turnDirection = -turnDirection;
    }
}

void KillRoomScene::updateMonsters(DoomEngine& engine, const ParameterMap& params,
                                    float deltaTime)
{
    spawnCooldown -= deltaTime;

    // Age all monsters
    for (auto& m : monsters)
        m.lifetime += deltaTime;

    // Remove expired monsters first
    for (auto it = monsters.begin(); it != monsters.end();)
    {
        if (it->lifetime > kMonsterLifetime)
        {
            engine.removeThing(it->handle);
            it = monsters.erase(it);
        }
        else
            ++it;
    }

    // Spawn on onset trigger — always in front of the player's view
    float spawnTrigger = getParam(params, "monster_spawn", 0.0f);

    if (spawnTrigger > 0.5f && spawnCooldown <= 0.0f)
    {
        // Evict oldest if at limit
        while (static_cast<int>(monsters.size()) >= kMaxMonsters && !monsters.empty())
        {
            engine.removeThing(monsters.front().handle);
            monsters.erase(monsters.begin());
        }

        // Spawn directly in front of player, 200-300 map units ahead
        float angleRad = static_cast<float>(camAngle) * (2.0f * 3.14159265f / 4294967296.0f);
        float dist = 200.0f + std::fmod(spawnCooldown * 1000.0f, 100.0f);
        int32_t spawnX = camX + static_cast<int32_t>(std::cos(angleRad) * dist * 65536.0f);
        int32_t spawnY = camY + static_cast<int32_t>(std::sin(angleRad) * dist * 65536.0f);

        int typeIdx = static_cast<int>(monsters.size()) % kNumMonsterTypes;

        int handle = engine.spawnThing(spawnX, spawnY, kMonsterTypes[typeIdx]);
        if (handle >= 0)
        {
            monsters.push_back({handle, 0.0f});
            spawnCooldown = kSpawnCooldown;
        }
    }
}

void KillRoomScene::updateLighting(DoomEngine& engine, const ParameterMap& params)
{
    // Audio drives lighting; default to moderate if no audio
    float lightLevel = getParam(params, "sector_light.all", 0.5f);

    for (size_t i = 0; i < outdoorSectors.size(); ++i)
    {
        int base = baseLightLevels[i];
        int modulated = static_cast<int>(static_cast<float>(base) * (0.3f + 0.7f * lightLevel));
        modulated = std::clamp(modulated, 0, 255);
        engine.setSectorLight(outdoorSectors[i], modulated);
    }
}

const uint8_t* KillRoomScene::render(DoomEngine& engine)
{
    return engine.renderFrame();
}

void KillRoomScene::updateCombat(DoomEngine& engine, const ParameterMap& params, float deltaTime)
{
    combatCooldown -= deltaTime;
    if (combatCooldown > 0.0f || monsters.empty())
        return;

    // Only shoot when there's audio energy
    float energy = getParam(params, "sector_light.all", 0.0f);
    if (energy < 0.05f)
        return;

    // Find nearest monster
    int32_t nearestDist = 0x7FFFFFFF;
    int nearestIdx = -1;

    for (size_t i = 0; i < monsters.size(); ++i)
    {
        int32_t mx, my;
        if (!engine.getThingPosition(monsters[i].handle, mx, my))
            continue;

        int32_t ddx = (mx - camX) >> 16; // convert to map units
        int32_t ddy = (my - camY) >> 16;
        int32_t dist = ddx * ddx + ddy * ddy; // squared distance

        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearestIdx = static_cast<int>(i);
        }
    }

    // If a monster is within ~300 map units, face it and shoot
    // 300^2 = 90000
    if (nearestIdx >= 0 && nearestDist < 90000)
    {
        int32_t mx, my;
        engine.getThingPosition(monsters[static_cast<size_t>(nearestIdx)].handle, mx, my);

        // Calculate angle to monster
        float ddx = static_cast<float>((mx - camX) >> 16);
        float ddy = static_cast<float>((my - camY) >> 16);
        float targetAngle = std::atan2(ddy, ddx);
        camAngle = static_cast<uint32_t>(targetAngle / (2.0f * 3.14159265f) * 4294967296.0f);

        engine.fireWeapon();
        combatCooldown = 0.3f; // fire rate
    }
}

void KillRoomScene::cleanup(DoomEngine& engine)
{
    for (auto& m : monsters)
        engine.removeThing(m.handle);
    monsters.clear();

    for (size_t i = 0; i < outdoorSectors.size(); ++i)
        engine.setSectorLight(outdoorSectors[i], baseLightLevels[i]);
}
