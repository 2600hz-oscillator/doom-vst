#pragma once

#include "Scene.h"
#include "patch/VisualizerState.h"
#include <vector>
#include <array>
#include <cstdint>

class AudioAnalyzer;

// Scene A: Auto-navigating kill room in E1M1 outdoor area.
// Audio drives lighting, per-band rising edges spawn things matching the
// global sprite assignment (monsters, powerups, armor — guns skipped).
class KillRoomScene : public Scene
{
public:
    KillRoomScene(const AudioAnalyzer& analyzer,
                  const patch::VisualizerState& vizState);

    void init(DoomEngine& engine) override;
    void update(DoomEngine& engine, const ParameterMap& params, float deltaTime) override;
    const uint8_t* render(DoomEngine& engine) override;
    std::string getName() const override { return "kill_room"; }
    void cleanup(DoomEngine& engine) override;

private:
    const AudioAnalyzer& analyzer;
    const patch::VisualizerState& vizState;

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
    static constexpr int kMaxMonsters = 50;
    static constexpr float kMonsterLifetime = 15.0f;

    // Per-band amplitude tracking and rising-edge spawn detection.
    // Bands come from the global config (same edges as Spectrum + Analyzer);
    // each band runs its own envelope follower + edge detector + cooldown
    // so a sustained loud band doesn't flood the map.
    static constexpr int   kNumBands         = patch::kNumBands;
    static constexpr float kSpawnHi          = 0.5f;  // rising-edge threshold
    static constexpr float kSpawnLo          = 0.4f;  // hysteresis low edge
    static constexpr float kPerBandCooldown  = 0.5f;  // sec between spawns per band

    std::array<float, kNumBands> bandAmplitudes {};
    std::array<bool,  kNumBands> bandAboveThreshold {};
    std::array<float, kNumBands> bandSpawnCooldown {};

    // Outdoor sector IDs (E1M1 sky sectors)
    std::vector<int> outdoorSectors;
    std::vector<int> baseLightLevels;

    void findOutdoorSectors(DoomEngine& engine);
    void updateBands(float deltaTime);
    void updateCamera(DoomEngine& engine, const ParameterMap& params, float deltaTime);
    void updateSpawning(DoomEngine& engine, float deltaTime);
    void updateLighting(DoomEngine& engine, const ParameterMap& params);
    void updateCombat(DoomEngine& engine, const ParameterMap& params, float deltaTime);

    float combatCooldown = 0.0f;
};
