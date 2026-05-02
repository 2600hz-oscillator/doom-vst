// vrt_runner.cpp — Visual Regression Test runner.
//
// Drives a single DoomViz scene against a deterministic WAV input, in fixed
// 512-sample blocks at 44.1 kHz, and writes 320×200 PPM snapshots every Nth
// block to <output_dir>. Snapshots are then compared to committed baselines
// by the Python harness (test/vrt/run_vrt.py).
//
// Usage:
//   doomviz_vrt_runner <scene> <input.wav> <output_dir> <wad_path> [config.yaml]
//
//   scene       killroom | analyzer | spectrum2
//   wad_path    Path to DOOM1.WAD (the harness passes resources/DOOM1.WAD)
//   config.yaml Optional routing config for KillRoom (defaults to all-zero
//               params if omitted, which is fine for VRT — the routed values
//               only affect KillRoom's monster spawning + camera).

#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include "doom/DoomEngine.h"
#include "patch/VisualizerState.h"
#include "routing/RouteConfig.h"
#include "routing/SignalRouter.h"
#include "scenes/AnalyzerRoomScene.h"
#include "scenes/KillRoomScene.h"
#include "scenes/Spectrum2Scene.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{
    constexpr int kBlockSize       = 512;
    constexpr int kSnapshotEveryN  = 43;     // ≈ 0.5 s at 44.1 kHz / 512 = 86 fps
    constexpr int kFrameW          = 320;
    constexpr int kFrameH          = 200;

    void writePPM(const std::string& path, const uint8_t* rgba)
    {
        FILE* f = std::fopen(path.c_str(), "wb");
        if (! f) { std::fprintf(stderr, "Cannot open %s\n", path.c_str()); std::exit(2); }
        std::fprintf(f, "P6\n%d %d\n255\n", kFrameW, kFrameH);
        for (int i = 0; i < kFrameW * kFrameH; ++i)
        {
            std::fputc(rgba[i * 4 + 0], f);
            std::fputc(rgba[i * 4 + 1], f);
            std::fputc(rgba[i * 4 + 2], f);
        }
        std::fclose(f);
    }

    std::unique_ptr<Scene> makeScene(const std::string& name,
                                     const AudioAnalyzer& analyzer,
                                     const SignalBus& bus,
                                     const patch::VisualizerState& vizState)
    {
        if (name == "killroom")  return std::make_unique<KillRoomScene>(analyzer, vizState);
        if (name == "analyzer")  return std::make_unique<AnalyzerRoomScene>(analyzer, bus);
        if (name == "spectrum2") return std::make_unique<Spectrum2Scene>(analyzer, vizState);
        std::fprintf(stderr, "Unknown scene '%s'. Use killroom | analyzer | spectrum2.\n",
                     name.c_str());
        std::exit(2);
    }
}

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        std::fprintf(stderr,
            "Usage: %s <scene> <input.wav> <output_dir> <wad_path> [config.yaml]\n", argv[0]);
        return 2;
    }

    const std::string sceneName = argv[1];
    const std::string wavPath   = argv[2];
    const std::string outDir    = argv[3];
    const std::string wadPath   = argv[4];
    const std::string cfgPath   = (argc >= 6) ? argv[5] : "";

    // Make sure the output directory exists before we try to write into it.
    juce::File(outDir).createDirectory();

    // --- Load WAV ---
    juce::AudioFormatManager fmgr;
    fmgr.registerBasicFormats();
    juce::File wavFile(wavPath);
    std::unique_ptr<juce::AudioFormatReader> reader(fmgr.createReaderFor(wavFile));
    if (! reader)
    {
        std::fprintf(stderr, "Failed to read WAV: %s\n", wavPath.c_str());
        return 1;
    }
    const int    numCh      = (int) reader->numChannels;
    const int    numSamples = (int) reader->lengthInSamples;
    const double sampleRate = reader->sampleRate;
    juce::AudioBuffer<float> audio(numCh, numSamples);
    reader->read(&audio, 0, numSamples, 0, true, true);

    // --- Init Doom engine ---
    DoomEngine engine;
    if (! engine.init(wadPath))
    {
        std::fprintf(stderr, "Failed to init DoomEngine with WAD: %s\n", wadPath.c_str());
        return 1;
    }
    if (! engine.loadMap(1, 1))
    {
        std::fprintf(stderr, "Failed to load E1M1\n");
        return 1;
    }

    // --- Audio analysis chain ---
    SignalBus bus;
    AudioAnalyzer analyzer;
    analyzer.setSampleRate(sampleRate);

    SignalRouter router;
    if (! cfgPath.empty())
    {
        RouteConfig cfg;
        if (cfg.loadFromString(juce::File(cfgPath).loadFileAsString().toStdString()))
            router.loadConfig(cfg.getConfig());
    }

    patch::VisualizerState vizState;

    // --- Scene ---
    auto scene = makeScene(sceneName, analyzer, bus, vizState);
    scene->init(engine);

    // --- Run blocks, snapshot every Nth ---
    const int numBlocks = numSamples / kBlockSize;
    const float deltaTime = static_cast<float>(kBlockSize) / static_cast<float>(sampleRate);

    int snapshotIdx = 0;
    std::vector<float> mono(kBlockSize, 0.0f);

    for (int b = 0; b < numBlocks; ++b)
    {
        const int s0 = b * kBlockSize;

        // Mono downmix into the bus + analyzer (mirrors processBlock's path).
        if (numCh == 1)
        {
            std::memcpy(mono.data(), audio.getReadPointer(0) + s0, kBlockSize * sizeof(float));
        }
        else
        {
            const float invCh = 1.0f / static_cast<float>(numCh);
            for (int i = 0; i < kBlockSize; ++i)
            {
                float sum = 0.0f;
                for (int c = 0; c < numCh; ++c) sum += audio.getReadPointer(c)[s0 + i];
                mono[i] = sum * invCh;
            }
        }
        bus.pushAudioSamples(mono.data(), kBlockSize);
        analyzer.pushSamples(mono.data(), kBlockSize);

        router.evaluate(analyzer, bus, deltaTime);
        scene->update(engine, router.getParameters(), deltaTime);
        const uint8_t* rgba = scene->render(engine);

        if (b % kSnapshotEveryN == 0)
        {
            char fname[64];
            std::snprintf(fname, sizeof(fname), "frame_%04d.ppm", snapshotIdx++);
            writePPM(outDir + "/" + fname, rgba);
        }
    }

    std::printf("Wrote %d snapshots to %s\n", snapshotIdx, outDir.c_str());
    return 0;
}
