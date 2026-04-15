#pragma once

#include "RouteConfig.h"
#include <unordered_map>
#include <string>

class AudioAnalyzer;
class SignalBus;

// Evaluates all routes each frame, reading from AudioAnalyzer and SignalBus,
// producing a destination parameter map that scenes read to drive visuals.
class SignalRouter
{
public:
    SignalRouter();

    // Load a scene configuration.
    void loadConfig(const SceneConfig& config);

    // Evaluate all routes for this frame.
    // Call from render thread after AudioAnalyzer has been updated.
    void evaluate(const AudioAnalyzer& analyzer, const SignalBus& bus, float deltaTime);

    // Read a destination parameter value (0.0 default if not routed).
    float getParameter(const std::string& name) const;

    // Get the full parameter map (for scenes that iterate all params).
    const std::unordered_map<std::string, float>& getParameters() const { return parameters; }

    // Check if a config is loaded.
    bool hasConfig() const { return configLoaded; }

private:
    SceneConfig config;
    bool configLoaded = false;

    // Per-input smoothed values
    std::unordered_map<std::string, float> smoothedInputs;

    // Output parameter map
    std::unordered_map<std::string, float> parameters;

    float evaluateInput(const InputChannel& ch, const AudioAnalyzer& analyzer,
                        const SignalBus& bus) const;

    int bandIndexForFreq(float freq, const AudioAnalyzer& analyzer) const;
};
