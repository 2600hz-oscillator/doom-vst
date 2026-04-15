#include "SignalRouter.h"
#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include <cmath>
#include <algorithm>

SignalRouter::SignalRouter() = default;

void SignalRouter::loadConfig(const SceneConfig& cfg)
{
    config = cfg;
    configLoaded = true;
    smoothedInputs.clear();
    parameters.clear();

    for (const auto& ch : config.inputs)
        smoothedInputs[ch.name] = 0.0f;
}

float SignalRouter::evaluateInput(const InputChannel& ch,
                                   const AudioAnalyzer& analyzer,
                                   const SignalBus& bus) const
{
    float raw = 0.0f;

    switch (ch.mode)
    {
        case InputChannel::Mode::FullRMS:
            raw = analyzer.getRMSLevel();
            break;

        case InputChannel::Mode::BandRMS:
        {
            // Map frequency range to band indices and average
            const float* bands = analyzer.getBandRMS();
            int numBands = analyzer.getNumBands();
            int lo = bandIndexForFreq(ch.bandLow, analyzer);
            int hi = bandIndexForFreq(ch.bandHigh, analyzer);
            lo = std::clamp(lo, 0, numBands - 1);
            hi = std::clamp(hi, lo, numBands - 1);

            float sum = 0.0f;
            for (int i = lo; i <= hi; ++i)
                sum += bands[i];
            raw = (hi >= lo) ? sum / static_cast<float>(hi - lo + 1) : 0.0f;
            break;
        }

        case InputChannel::Mode::OnsetDetect:
            raw = analyzer.getOnset() ? 1.0f : 0.0f;
            break;

        case InputChannel::Mode::NoteVelocity:
            raw = static_cast<float>(bus.getLastVelocity()) / 127.0f;
            break;

        case InputChannel::Mode::CC:
            raw = static_cast<float>(bus.getCC(ch.ccNumber)) / 127.0f;
            break;

        case InputChannel::Mode::ClockBPM:
        {
            // Rough BPM from clock count (24 ppqn)
            // This is a placeholder — proper BPM needs timing between clocks
            raw = static_cast<float>(bus.getMidiClockCount()) * 0.01f;
            break;
        }
    }

    // Apply gain and offset
    return std::clamp(raw * ch.gain + ch.offset, 0.0f, 1.0f);
}

int SignalRouter::bandIndexForFreq(float freq, const AudioAnalyzer& analyzer) const
{
    // Map frequency to the nearest band index (log-spaced 16 bands 20-20kHz)
    int numBands = analyzer.getNumBands();
    if (freq <= 20.0f) return 0;
    if (freq >= 20000.0f) return numBands - 1;

    float logMin = std::log2(20.0f);
    float logMax = std::log2(20000.0f);
    float logFreq = std::log2(freq);
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return std::clamp(static_cast<int>(normalized * static_cast<float>(numBands)),
                      0, numBands - 1);
}

void SignalRouter::evaluate(const AudioAnalyzer& analyzer, const SignalBus& bus,
                             float deltaTime)
{
    if (!configLoaded)
        return;

    // Evaluate and smooth each input channel
    for (const auto& ch : config.inputs)
    {
        float raw = evaluateInput(ch, analyzer, bus);

        // Exponential smoothing
        float& smoothed = smoothedInputs[ch.name];
        if (ch.smoothing > 0.0f && deltaTime > 0.0f)
        {
            float coeff = 1.0f - std::exp(-deltaTime / ch.smoothing);
            smoothed += coeff * (raw - smoothed);
        }
        else
        {
            smoothed = raw;
        }
    }

    // Clear parameters for replace mode, keep for additive
    for (auto& [name, val] : parameters)
        val = 0.0f;

    // Evaluate each route
    for (const auto& route : config.routes)
    {
        auto it = smoothedInputs.find(route.from);
        if (it == smoothedInputs.end())
            continue;

        float value = it->second * route.scale + route.offset;
        value = std::clamp(value, 0.0f, 1.0f);

        switch (route.blend)
        {
            case Route::Blend::Replace:
                parameters[route.to] = value;
                break;
            case Route::Blend::Add:
                parameters[route.to] += value;
                break;
            case Route::Blend::Multiply:
                parameters[route.to] = parameters.count(route.to)
                    ? parameters[route.to] * value : value;
                break;
            case Route::Blend::Max:
                parameters[route.to] = std::max(parameters[route.to], value);
                break;
        }
    }
}

float SignalRouter::getParameter(const std::string& name) const
{
    auto it = parameters.find(name);
    return it != parameters.end() ? it->second : 0.0f;
}
