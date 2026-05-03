#include "AnalyzerRoomScene.h"
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include "doom_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

static constexpr int WP_BFG = 6;
static constexpr float PI = 3.14159265f;

AnalyzerRoomScene::AnalyzerRoomScene(const AudioAnalyzer& az,
                                      const SignalBus& bus,
                                      const patch::VisualizerState& state)
    : analyzer(az), signalBus(bus), vizState(state)
{
}

void AnalyzerRoomScene::init(DoomEngine& engine)
{
    engine.getPlayerPos(camX, camY, camZ, camAngle);
    walkSpeed = 400.0f;
    turnTimer = 0.0f;
    turnDirection = 1.0f;
    blockedTimer = 0.0f;
    bandEnv.reset();

    engine.setGodMode(true);
    engine.giveWeapon(WP_BFG);

    if (!texInitialized)
    {
        engine.setWallTextureData(kTargetTexture, nullptr, &texWidth, &texHeight);
        if (texWidth > 0 && texHeight > 0)
        {
            texPixels.resize(static_cast<size_t>(texWidth * texHeight), 0);
            texInitialized = true;
        }
    }
}

void AnalyzerRoomScene::updateBands(float deltaTime)
{
    // FFT bins → 8 user-configured band envelopes (shared with Spectrum2 +
    // KillRoom).
    bandEnv.update(analyzer, vizState.getGlobal(), deltaTime);
}

void AnalyzerRoomScene::updateNavigation(DoomEngine& engine, float deltaTime)
{
    turnTimer += deltaTime;

    // Periodically change gentle turn direction
    if (turnTimer > 5.0f)
    {
        turnTimer = 0.0f;
        turnDirection = -turnDirection;
    }

    // If blocked, turn in place until clear
    if (blockedTimer > 0.0f)
    {
        blockedTimer -= deltaTime;
        camAngle += static_cast<uint32_t>(turnDirection * 500000000.0f * deltaTime);
        engine.setCameraAngle(camAngle);
        return;
    }

    // Gentle ambient turn
    float gentleTurn = turnDirection * 0.15f;
    camAngle += static_cast<uint32_t>(gentleTurn * 100000000.0f * deltaTime);
    engine.setCameraAngle(camAngle);

    // Movement speed driven by the first band (sub bass / red bar)
    float bassLevel = bandEnv[0];
    float currentSpeed = walkSpeed * bassLevel;

    float angleRad = static_cast<float>(camAngle) * (2.0f * PI / 4294967296.0f);
    float dist = currentSpeed * deltaTime;
    int32_t dx = static_cast<int32_t>(std::cos(angleRad) * dist * 65536.0f);
    int32_t dy = static_cast<int32_t>(std::sin(angleRad) * dist * 65536.0f);

    if (engine.movePlayer(dx, dy))
    {
        // Move succeeded — read back new position (movePlayer updates mobj)
        engine.getPlayerPos(camX, camY, camZ, camAngle);
    }
    else
    {
        blockedTimer = 0.4f + 0.3f * std::fmod(turnTimer * 7.0f, 1.0f);
        turnDirection = -turnDirection;
    }
}

void AnalyzerRoomScene::update(DoomEngine& engine, const ParameterMap& params,
                                float deltaTime)
{
    (void)params;

    if (engine.isPlayerDead())
    {
        engine.respawnPlayer();
        engine.getPlayerPos(camX, camY, camZ, camAngle);
        engine.setGodMode(true);
    }

    updateBands(deltaTime);
    updateNavigation(engine, deltaTime);

    renderFFTToTexture();

    if (texInitialized)
        engine.setWallTextureData(kTargetTexture, texPixels.data(), nullptr, nullptr);
}

void AnalyzerRoomScene::renderFFTToTexture()
{
    if (!texInitialized || texWidth <= 0 || texHeight <= 0)
        return;

    std::memset(texPixels.data(), 0, texPixels.size());

    int barW = texWidth / kNumBars;

    for (int i = 0; i < kNumBars; ++i)
    {
        float amp = bandEnv[static_cast<size_t>(i)];
        int barH = std::clamp(static_cast<int>(amp * static_cast<float>(texHeight)),
                              0, texHeight - 1);

        uint8_t color = kBarPalette[i];

        int colStart = i * barW;
        int colEnd = std::min(colStart + barW - 1, texWidth);

        for (int col = colStart; col < colEnd; ++col)
        {
            for (int row = 0; row < barH; ++row)
            {
                int y = texHeight - 1 - row;
                texPixels[static_cast<size_t>(col * texHeight + y)] = color;
            }
        }
    }
}

const uint8_t* AnalyzerRoomScene::render(DoomEngine& engine)
{
    return engine.renderFrame();
}

void AnalyzerRoomScene::cleanup(DoomEngine& engine)
{
    (void)engine;
}
