#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// Describes an input analysis channel
struct InputChannel
{
    std::string name;

    enum class Source { Audio, Midi };
    Source source = Source::Audio;

    enum class Mode {
        BandRMS,        // RMS of a frequency band
        FullRMS,        // Overall RMS level
        OnsetDetect,    // Onset trigger (0 or 1)
        NoteVelocity,   // Last MIDI note velocity (0-1)
        CC,             // MIDI CC value (0-1)
        ClockBPM        // Derived BPM from MIDI clock
    };
    Mode mode = Mode::FullRMS;

    // For BandRMS: frequency range
    float bandLow = 20.0f;
    float bandHigh = 20000.0f;

    // For CC mode
    int ccNumber = 1;

    // For NoteVelocity mode
    int midiChannel = 0; // 0 = any

    // Processing
    float gain = 1.0f;
    float offset = 0.0f;
    float smoothing = 0.05f; // seconds (slew)
};

// Describes a route from input to destination parameter
struct Route
{
    std::string from;   // Input channel name
    std::string to;     // Destination parameter name

    float scale = 1.0f;
    float offset = 0.0f;

    enum class Blend { Replace, Add, Multiply, Max };
    Blend blend = Blend::Replace;
};

// Parsed configuration for a scene
struct SceneConfig
{
    std::string sceneName;
    std::vector<InputChannel> inputs;
    std::vector<Route> routes;
};

// Parses a YAML config file into a SceneConfig.
class RouteConfig
{
public:
    // Parse from a YAML file path. Returns true on success.
    bool loadFromFile(const std::string& path);

    // Parse from a YAML string.
    bool loadFromString(const std::string& yamlContent);

    const SceneConfig& getConfig() const { return config; }
    const std::string& getError() const { return errorMsg; }

private:
    SceneConfig config;
    std::string errorMsg;
};
