#include "AnalyzerRoomScene.h"
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include "doom_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// BFG9000 weapon ID
static constexpr int WP_BFG = 6;

AnalyzerRoomScene::AnalyzerRoomScene(const AudioAnalyzer& az, const SignalBus& bus)
    : analyzer(az), signalBus(bus)
{
}

void AnalyzerRoomScene::init(DoomEngine& engine)
{
    engine.getPlayerPos(camX, camY, camZ, camAngle);
    bpm = 120.0f;
    lastClockCount = 0;

    engine.setGodMode(true);
    engine.giveWeapon(WP_BFG);

    // Get the wall texture dimensions
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

void AnalyzerRoomScene::update(DoomEngine& engine, const ParameterMap& params,
                                float deltaTime)
{
    (void)params;

    // BPM from MIDI clock (24 ppqn)
    int currentClock = signalBus.getMidiClockCount();
    if (currentClock != lastClockCount)
    {
        int ticksDelta = currentClock - lastClockCount;
        lastClockCount = currentClock;

        if (ticksDelta > 0 && deltaTime > 0.0f)
        {
            float ticksPerSec = static_cast<float>(ticksDelta) / deltaTime;
            float measuredBpm = (ticksPerSec / 24.0f) * 60.0f;
            if (measuredBpm > 30.0f && measuredBpm < 300.0f)
                bpm = bpm * 0.9f + measuredBpm * 0.1f;
        }
    }

    // Rotate camera: BPM degrees/sec (360 BPM = 1 revolution/sec)
    float degreesPerSec = bpm;
    uint32_t angleDelta = static_cast<uint32_t>(
        degreesPerSec / 360.0f * 4294967296.0f * deltaTime);
    camAngle += angleDelta;

    // Render FFT to the wall texture
    renderFFTToTexture();

    // Inject texture into Doom
    if (texInitialized)
        engine.setWallTextureData(kTargetTexture, texPixels.data(), nullptr, nullptr);

    // Set camera (stationary, just rotating)
    engine.setCamera(camX, camY, camZ, camAngle);
}

void AnalyzerRoomScene::renderFFTToTexture()
{
    if (!texInitialized || texWidth <= 0 || texHeight <= 0)
        return;

    // Clear to dark
    std::memset(texPixels.data(), 0, texPixels.size());

    // Get band envelope values
    const float* envelope = analyzer.getBandEnvelope();
    int analyzerBands = analyzer.getNumBands();

    int barW = texWidth / kNumBars;

    for (int i = 0; i < kNumBars; ++i)
    {
        // Map analyzer bands to our 8 display bands
        int srcIdx = i * analyzerBands / kNumBars;
        int srcEnd = (i + 1) * analyzerBands / kNumBars;
        float amp = 0.0f;
        int count = 0;
        for (int j = srcIdx; j < srcEnd && j < analyzerBands; ++j)
        {
            amp += envelope[j];
            count++;
        }
        if (count > 0) amp /= static_cast<float>(count);

        int barH = std::clamp(static_cast<int>(amp * static_cast<float>(texHeight)),
                              0, texHeight - 1);

        uint8_t color = kBarPalette[i];

        // Draw column-major: for each column in this bar's range
        int colStart = i * barW;
        int colEnd = std::min(colStart + barW - 1, texWidth);

        for (int col = colStart; col < colEnd; ++col)
        {
            // Column data is at texPixels[col * texHeight ... col * texHeight + texHeight-1]
            // Draw bar from bottom up
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
