#pragma once

#include <array>
#include <vector>
#include <juce_core/juce_core.h>

namespace patch
{

// Per-band config (global across all scenes).
struct Band
{
    float lowHz;
    float highHz;
    float gain01;   // [0, 1]
    int   spriteId; // matches SPR_* enum order in linuxdoom info.h
};

constexpr int   kNumBands           = 8;
constexpr float kSpectrumMaxGain    = 4.0f;

enum class BackgroundVibe : int
{
    AcidwarpExe = 0,
    Vaporwave   = 1,
    Punkrock    = 2,
    Doomtex     = 3,
    Winamp      = 4,
    Starfield   = 5,
    Crtglitch   = 6,
};

constexpr int kNumBackgroundVibes = 7;

// Global frequency-band configuration. Shared across Spectrum, KillRoom,
// and Analyzer scenes — the user defines bands once and every scene uses
// them.
struct GlobalConfig
{
    std::array<Band, kNumBands> bands;

    static GlobalConfig makeDefault()
    {
        // Hz edges replicate the prior hardcoded 8-from-16 mapping derived
        // from AudioAnalyzer's log-spaced kBandEdges. gain01 = 0.75 paired
        // with kSpectrumMaxGain = 4.0 gives a peak sprite scale of 3.0 at
        // band amplitude 1.0, matching the prior `0.5 + amp * 2.5` peak.
        // Default sprite per band matches the prior hardcoded `kBandSprites`:
        // PLAY (28), PLAY (28), TROO (0) x4, POSS (29), SPOS (30).
        GlobalConfig g {};
        g.bands = { {
            { 20.0f,    80.0f,    0.75f, 28 },
            { 80.0f,    315.0f,   0.75f, 28 },
            { 315.0f,   1250.0f,  0.75f,  0 },
            { 1250.0f,  5000.0f,  0.75f,  0 },
            { 5000.0f,  10000.0f, 0.75f,  0 },
            { 10000.0f, 14000.0f, 0.75f,  0 },
            { 14000.0f, 18000.0f, 0.75f, 29 },
            { 18000.0f, 20000.0f, 0.75f, 30 },
        } };
        return g;
    }
};

// Spectrum2Scene-specific config.
struct SpectrumConfig
{
    BackgroundVibe vibe { BackgroundVibe::AcidwarpExe };

    // DOOMTEX background controls. doomtexIndex is the manual selection
    // (or seed when auto-advance is on). When auto-advance is on, the
    // scene cycles internally; this field stays at the last user-applied
    // starting texture.
    //
    // Defaults match the previous hard-coded behavior:
    //   • auto-advance ON
    //   • band 3 drives advance (1-indexed; matches old `bandAmplitudes[2]`)
    //   • threshold 0.5 (matches old kHi=0.5; hysteresis low edge is
    //     computed as max(0, threshold-0.1) to match old kLo=0.4 spread)
    int   doomtexIndex          { 0 };
    bool  doomtexAutoAdvance    { true };
    int   doomtexAutoBand       { 3 };    // 1..kNumBands
    float doomtexAutoThreshold  { 0.5f }; // 0..1

    static SpectrumConfig makeDefault() { return {}; }
};

// KillRoomScene-specific config (placeholder — populated in later phases).
struct KillRoomConfig
{
    static KillRoomConfig makeDefault() { return {}; }
};

// AnalyzerRoomScene-specific config (placeholder — populated in later phases).
struct AnalyzerConfig
{
    static AnalyzerConfig makeDefault() { return {}; }
};

// One sampler pad. `sampleData` is decoded float mono at the host sample
// rate captured in `sourceSampleRate` at load time. An empty `sampleData`
// means the pad has no sample loaded.
struct PadConfig
{
    juce::String       name;             // filename or "" if empty
    std::vector<float> sampleData;       // mono, host SR at load time
    double             sourceSampleRate; // sample rate the buffer is at
    int                startSample;      // playback start (inclusive)
    int                endSample;        // playback end (exclusive)

    static PadConfig makeDefault() { return { {}, {}, 0.0, 0, 0 }; }
};

constexpr int kNumPads = 9;

// 9-pad sampler state (BFG-SP404). MIDI channels 1-9 each address one pad;
// triggers run through SamplerEngine on the audio thread.
struct SamplerConfig
{
    std::array<PadConfig, kNumPads> pads;

    static SamplerConfig makeDefault()
    {
        SamplerConfig s {};
        for (auto& p : s.pads) p = PadConfig::makeDefault();
        return s;
    }
};

// Persistent shared state for the whole visualizer. GUI thread writes
// (Apply button); render thread reads (per-frame snapshot copy under
// SpinLock). The lock is only contended during a click, so the render-
// thread copy is essentially free.
//
// Configuration persists across scene switches — only transient/animation
// state (which lives in the scenes themselves) resets on init().
class VisualizerState
{
public:
    VisualizerState();

    GlobalConfig   getGlobal()   const;
    void           setGlobal(const GlobalConfig& g);

    SpectrumConfig getSpectrum() const;
    void           setSpectrum(const SpectrumConfig& s);

    KillRoomConfig getKillRoom() const;
    void           setKillRoom(const KillRoomConfig& k);

    AnalyzerConfig getAnalyzer() const;
    void           setAnalyzer(const AnalyzerConfig& a);

    SamplerConfig  getSampler() const;
    void           setSampler(const SamplerConfig& s);

    // Serialize/restore as XML (for AudioProcessor state save/restore).
    juce::String toXmlString() const;
    void         fromXmlString(const juce::String& xml);

private:
    mutable juce::SpinLock lock;
    GlobalConfig   global   { GlobalConfig::makeDefault() };
    SpectrumConfig spectrum { SpectrumConfig::makeDefault() };
    KillRoomConfig killRoom { KillRoomConfig::makeDefault() };
    AnalyzerConfig analyzer { AnalyzerConfig::makeDefault() };
    SamplerConfig  sampler  { SamplerConfig::makeDefault() };
};

} // namespace patch
