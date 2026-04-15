#include "AnalyzerRoomScene.h"
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "doom_renderer.h"
#include <cmath>
#include <algorithm>

AnalyzerRoomScene::AnalyzerRoomScene(const AudioAnalyzer& az)
    : analyzer(az)
{
}

void AnalyzerRoomScene::init(DoomEngine& engine)
{
    engine.getPlayerPos(camX, camY, camZ, camAngle);
    turnTimer = 0.0f;
    turnDirection = 1.0f;
    blockedTimer = 0.0f;
    flatPixels.fill(0);
    engine.setGodMode(true);
}

void AnalyzerRoomScene::update(DoomEngine& engine, const ParameterMap& params,
                                float deltaTime)
{
    if (engine.isPlayerDead())
    {
        engine.respawnPlayer();
        engine.getPlayerPos(camX, camY, camZ, camAngle);
        engine.setGodMode(true);
    }

    updateCamera(engine, params, deltaTime);
    renderFFTToFlat();

    // Inject the FFT flat into Doom's texture cache
    doom_set_flat_data("NUKAGE1", flatPixels.data());

    engine.setCamera(camX, camY, camZ, camAngle);

    // Update sector lighting from params
    float lightLevel = 0.5f;
    auto it = params.find("sector_light.all");
    if (it != params.end()) lightLevel = it->second;

    doom_map_info_t info = doom_get_map_info();
    for (int i = 0; i < info.num_sectors; ++i)
    {
        if (doom_sector_is_outdoor(i))
        {
            int level = static_cast<int>(128.0f + 127.0f * lightLevel);
            engine.setSectorLight(i, std::clamp(level, 0, 255));
        }
    }

    engine.tick();
}

void AnalyzerRoomScene::updateCamera(DoomEngine& engine, const ParameterMap& params, float deltaTime)
{
    float speedParam = 0.3f;
    auto it = params.find("player_speed");
    if (it != params.end()) speedParam = it->second;

    float baseSpeed = 300.0f;
    float audioBoost = speedParam * 500.0f;
    float moveSpeed = (baseSpeed + audioBoost) * 65536.0f;

    turnTimer += deltaTime;
    if (turnTimer > 5.0f)
    {
        turnTimer = 0.0f;
        turnDirection = -turnDirection;
    }

    if (blockedTimer > 0.0f)
    {
        blockedTimer -= deltaTime;
        camAngle += static_cast<uint32_t>(turnDirection * 500000000.0f * deltaTime);
        engine.setCamera(camX, camY, camZ, camAngle);
        engine.getPlayerPos(camX, camY, camZ, camAngle);
        return;
    }

    camAngle += static_cast<uint32_t>(turnDirection * 0.15f * 100000000.0f * deltaTime);

    float angleRad = static_cast<float>(camAngle) * (2.0f * 3.14159265f / 4294967296.0f);
    int32_t dx = static_cast<int32_t>(std::cos(angleRad) * moveSpeed * deltaTime);
    int32_t dy = static_cast<int32_t>(std::sin(angleRad) * moveSpeed * deltaTime);

    if (!engine.movePlayer(dx, dy))
    {
        blockedTimer = 0.4f + 0.3f * (std::fmod(turnTimer * 7.0f, 1.0f));
        turnDirection = -turnDirection;
    }

    engine.setCamera(camX, camY, camZ, camAngle);
    engine.getPlayerPos(camX, camY, camZ, camAngle);
}

void AnalyzerRoomScene::renderFFTToFlat()
{
    // Clear flat
    flatPixels.fill(0);

    const float* spectrum = analyzer.getMagnitudeSpectrum();
    int specSize = analyzer.getSpectrumSize();

    // Draw FFT bars onto the 64x64 flat
    // Map spectrum bins to 64 columns, magnitude to row height
    int binsPerCol = std::max(1, specSize / 64);

    for (int col = 0; col < 64; ++col)
    {
        // Average bins for this column
        float mag = 0.0f;
        int startBin = col * binsPerCol;
        int endBin = std::min(startBin + binsPerCol, specSize);
        for (int bin = startBin; bin < endBin; ++bin)
            mag += spectrum[bin];
        mag /= static_cast<float>(endBin - startBin);

        // Scale magnitude to pixel height (0-63)
        int height = std::clamp(static_cast<int>(mag * 5000.0f), 0, 63);

        // Draw column from bottom up
        for (int row = 0; row < height; ++row)
        {
            int y = 63 - row; // bottom-up
            // Use green-to-red gradient based on height
            // Doom palette: greens around 112-127, reds around 176-191
            int palIdx;
            float normalized = static_cast<float>(row) / 63.0f;
            if (normalized < 0.5f)
                palIdx = 112 + static_cast<int>(normalized * 2.0f * 15.0f); // green
            else
                palIdx = 176 + static_cast<int>((normalized - 0.5f) * 2.0f * 15.0f); // red

            flatPixels[static_cast<size_t>(y * 64 + col)] = static_cast<uint8_t>(palIdx);
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
    flatPixels.fill(0);
}
