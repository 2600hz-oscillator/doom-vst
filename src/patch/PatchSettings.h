#pragma once

#include <array>

namespace patch
{

// Per-band config for the Spectrum scene: the slider attenuates how strongly
// the band's audio amplitude scales the sprite. gain01 == 0 → sprite hidden;
// gain01 == 1 → sprite scale = bandAmp * kSpectrumMaxGain.
struct SpectrumBand
{
    float lowHz;
    float highHz;
    float gain01;   // [0, 1]
    int   spriteId; // matches SPR_* enum order in linuxdoom info.h
};

constexpr int  kSpectrumNumBands = 8;
constexpr float kSpectrumMaxGain = 4.0f;

struct SpectrumSettings
{
    std::array<SpectrumBand, kSpectrumNumBands> bands;

    static SpectrumSettings makeDefault()
    {
        // Hz edges replicate the prior hardcoded 8-from-16 mapping derived
        // from AudioAnalyzer's log-spaced kBandEdges. gain01 = 0.75 paired
        // with kSpectrumMaxGain = 4.0 gives a peak sprite scale of 3.0 at
        // band amplitude 1.0, matching the prior `0.5 + amp * 2.5` peak so
        // default visuals stay close.
        // Default sprite per band matches the prior hardcoded `kBandSprites`
        // array: PLAY (28), PLAY (28), TROO (0), TROO (0), TROO (0), TROO (0),
        // POSS (29), SPOS (30).
        SpectrumSettings s {};
        s.bands = { {
            { 20.0f,    80.0f,    0.75f, 28 },
            { 80.0f,    315.0f,   0.75f, 28 },
            { 315.0f,   1250.0f,  0.75f,  0 },
            { 1250.0f,  5000.0f,  0.75f,  0 },
            { 5000.0f,  10000.0f, 0.75f,  0 },
            { 10000.0f, 14000.0f, 0.75f,  0 },
            { 14000.0f, 18000.0f, 0.75f, 29 },
            { 18000.0f, 20000.0f, 0.75f, 30 },
        } };
        return s;
    }
};

} // namespace patch
