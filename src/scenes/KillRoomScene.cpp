#include "KillRoomScene.h"
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "doom_renderer.h"
#include "patch/SpriteCatalog.h"
#include <cmath>
#include <algorithm>

namespace
{
    // Map a sprite ID from the global config to a Doom mobjtype_t value.
    // Returns -1 for sprites that should not spawn (guns, the player marine).
    //
    // mobjtype enum values come from libs/doom_renderer/doom_src/info.h —
    // they're stable across the Doom 1.10 source we vendored, so hard-coding
    // is safer than trying to compute them at runtime.
    int spriteToMobjType(int spriteId)
    {
        switch (spriteId)
        {
            // Characters (skip PLAY=28 — duplicate marines look incoherent)
            case  0: return 11;  // TROO → MT_TROOP        (Imp)
            case 29: return  1;  // POSS → MT_POSSESSED    (Zombieman)
            case 30: return  2;  // SPOS → MT_SHOTGUY
            case 39: return 12;  // SARG → MT_SERGEANT     (Demon)
            case 40: return 14;  // HEAD → MT_HEAD         (Cacodemon)
            case 42: return 15;  // BOSS → MT_BRUISER      (Baron of Hell)
            case 44: return 18;  // SKUL → MT_SKULL        (Lost Soul)

            // Powerups
            case 70: return 55;  // SOUL → MT_MISC12       (Soulsphere)
            case 71: return 56;  // PINV → MT_INV          (Invulnerability)
            case 72: return 57;  // PSTR → MT_MISC13       (Berserk)
            case 73: return 58;  // PINS → MT_INS          (Blursphere)
            case 75: return 59;  // SUIT → MT_MISC14       (Rad Suit)
            case 76: return 60;  // PMAP → MT_MISC15       (Computer Map)
            case 77: return 61;  // PVIS → MT_MISC16       (Light Amp Visor)

            // Armor
            case 55: return 43;  // ARM1 → MT_MISC0        (Green Armor)
            case 56: return 44;  // ARM2 → MT_MISC1        (Blue Armor)
            case 60: return 45;  // BON1 → MT_MISC2        (Health Bonus)
            case 61: return 46;  // BON2 → MT_MISC3        (Armor Bonus)

            // Guns (88 MGUN, 89 CSAW, 90 LAUN, 92 SHOT) and PLAY (28): skip.
            default: return -1;
        }
    }
}

KillRoomScene::KillRoomScene(const AudioAnalyzer& az,
                              const patch::VisualizerState& state)
    : analyzer(az), vizState(state)
{
}

void KillRoomScene::init(DoomEngine& engine)
{
    engine.getPlayerPos(camX, camY, camZ, camAngle);
    moveSpeed = 0.0f;
    turnTimer = 0.0f;
    blockedTimer = 0.0f;
    turnDirection = 1.0f;
    combatCooldown = 0.0f;
    monsters.clear();

    bandEnv.reset();
    bandAboveThreshold.fill(false);
    bandSpawnCooldown.fill(0.0f);

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

    updateBands(deltaTime);
    updateCombat(engine, params, deltaTime);
    updateCamera(engine, params, deltaTime);
    updateSpawning(engine, deltaTime);
    updateLighting(engine, params);

    engine.tick();
}

void KillRoomScene::updateBands(float deltaTime)
{
    // FFT bins → 8 user-configured band envelopes (shared with Spectrum2 +
    // AnalyzerRoom).
    bandEnv.update(analyzer, vizState.getGlobal(), deltaTime);

    // Tick down per-band spawn cooldowns.
    for (auto& c : bandSpawnCooldown)
        c = std::max(0.0f, c - deltaTime);
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

void KillRoomScene::updateSpawning(DoomEngine& engine, float deltaTime)
{
    (void)deltaTime;

    // Age existing things and reap expired ones.
    for (auto& m : monsters)
        m.lifetime += deltaTime;
    for (auto it = monsters.begin(); it != monsters.end();)
    {
        if (it->lifetime > kMonsterLifetime)
        {
            engine.removeThing(it->handle);
            it = monsters.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Per-band rising-edge spawn detector. Each band fires when its envelope
    // crosses kSpawnHi, then waits for the envelope to fall below kSpawnLo
    // before re-arming. A small per-band cooldown prevents rapid retriggering
    // even on noisy bands.
    patch::GlobalConfig global = vizState.getGlobal();
    int spawnsThisFrame = 0;
    constexpr int kMaxSpawnsPerFrame = 2;  // avoid bursty FFT frames flooding

    for (int b = 0; b < kNumBands; ++b)
    {
        float amp = bandEnv[static_cast<size_t>(b)];
        bool& above = bandAboveThreshold[static_cast<size_t>(b)];
        float& cd = bandSpawnCooldown[static_cast<size_t>(b)];

        if (! above && amp > kSpawnHi && cd <= 0.0f)
        {
            above = true;
            cd = kPerBandCooldown;

            int spriteId = global.bands[static_cast<size_t>(b)].spriteId;
            int mobjType = spriteToMobjType(spriteId);
            if (mobjType < 0)
                continue;  // gun or unknown — skip

            // Evict oldest if at the global cap.
            while (static_cast<int>(monsters.size()) >= kMaxMonsters && ! monsters.empty())
            {
                engine.removeThing(monsters.front().handle);
                monsters.erase(monsters.begin());
            }

            // Spawn in front of the player, 200..280 map units ahead. The
            // jitter (per-band offset) keeps multiple band hits from stacking
            // on the exact same point.
            float angleRad = static_cast<float>(camAngle) * (2.0f * 3.14159265f / 4294967296.0f);
            float dist = 200.0f + static_cast<float>(b) * 10.0f;
            int32_t spawnX = camX + static_cast<int32_t>(std::cos(angleRad) * dist * 65536.0f);
            int32_t spawnY = camY + static_cast<int32_t>(std::sin(angleRad) * dist * 65536.0f);

            int handle = engine.spawnThing(spawnX, spawnY, mobjType);
            if (handle >= 0)
            {
                monsters.push_back({ handle, 0.0f });
                if (++spawnsThisFrame >= kMaxSpawnsPerFrame)
                    break;
            }
        }
        else if (above && amp < kSpawnLo)
        {
            above = false;
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
