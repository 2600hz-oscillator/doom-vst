#include "RouteConfig.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

static InputChannel::Source parseSource(const std::string& s)
{
    if (s == "midi") return InputChannel::Source::Midi;
    return InputChannel::Source::Audio;
}

static InputChannel::Mode parseMode(const std::string& s)
{
    if (s == "band_rms")       return InputChannel::Mode::BandRMS;
    if (s == "full_rms")       return InputChannel::Mode::FullRMS;
    if (s == "onset_detect")   return InputChannel::Mode::OnsetDetect;
    if (s == "note_velocity")  return InputChannel::Mode::NoteVelocity;
    if (s == "cc")             return InputChannel::Mode::CC;
    if (s == "clock_bpm")      return InputChannel::Mode::ClockBPM;
    return InputChannel::Mode::FullRMS;
}

static Route::Blend parseBlend(const std::string& s)
{
    if (s == "add")      return Route::Blend::Add;
    if (s == "multiply") return Route::Blend::Multiply;
    if (s == "max")      return Route::Blend::Max;
    return Route::Blend::Replace;
}

bool RouteConfig::loadFromFile(const std::string& path)
{
    try
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            errorMsg = "Cannot open file: " + path;
            return false;
        }
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        return loadFromString(content);
    }
    catch (const std::exception& e)
    {
        errorMsg = e.what();
        return false;
    }
}

bool RouteConfig::loadFromString(const std::string& yamlContent)
{
    try
    {
        YAML::Node root = YAML::Load(yamlContent);
        config = SceneConfig{};

        if (root["scene"])
            config.sceneName = root["scene"].as<std::string>();

        // Parse inputs
        if (root["inputs"])
        {
            for (auto it = root["inputs"].begin(); it != root["inputs"].end(); ++it)
            {
                InputChannel ch;
                ch.name = it->first.as<std::string>();

                auto node = it->second;
                if (node["source"])
                    ch.source = parseSource(node["source"].as<std::string>());
                if (node["mode"])
                    ch.mode = parseMode(node["mode"].as<std::string>());
                if (node["band"] && node["band"].IsSequence() && node["band"].size() == 2)
                {
                    ch.bandLow = node["band"][0].as<float>();
                    ch.bandHigh = node["band"][1].as<float>();
                }
                if (node["cc_number"])
                    ch.ccNumber = node["cc_number"].as<int>();
                if (node["channel"])
                    ch.midiChannel = node["channel"].as<int>();
                if (node["gain"])
                    ch.gain = node["gain"].as<float>();
                if (node["offset"])
                    ch.offset = node["offset"].as<float>();
                if (node["smoothing"])
                    ch.smoothing = node["smoothing"].as<float>();

                config.inputs.push_back(std::move(ch));
            }
        }

        // Parse routes
        if (root["routes"])
        {
            for (const auto& rNode : root["routes"])
            {
                Route r;
                r.from = rNode["from"].as<std::string>();
                r.to = rNode["to"].as<std::string>();

                if (rNode["scale"])
                    r.scale = rNode["scale"].as<float>();
                if (rNode["offset"])
                    r.offset = rNode["offset"].as<float>();
                if (rNode["blend"])
                    r.blend = parseBlend(rNode["blend"].as<std::string>());

                config.routes.push_back(std::move(r));
            }
        }

        return true;
    }
    catch (const YAML::Exception& e)
    {
        errorMsg = std::string("YAML parse error: ") + e.what();
        return false;
    }
}
