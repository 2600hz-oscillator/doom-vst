#include "SampleSetIO.h"
#include "SampleResample.h"

#include <algorithm>
#include <cmath>

namespace patch
{

namespace
{
    // Standard RFC 4648 base64 via juce::Base64. Matches the Python
    // generator (scripts/build_default_sample_set.py) and any external
    // tool a user might use to author a .dvset.
    juce::String encodeSamplePCM(const std::vector<float>& samples)
    {
        if (samples.empty()) return {};
        std::vector<int16_t> pcm(samples.size());
        for (size_t i = 0; i < samples.size(); ++i)
        {
            float s = juce::jlimit(-1.0f, 1.0f, samples[i]);
            pcm[i] = static_cast<int16_t>(std::lround(s * 32767.0f));
        }
        juce::MemoryOutputStream mo;
        juce::Base64::convertToBase64(mo, pcm.data(),
                                       pcm.size() * sizeof(int16_t));
        return mo.toString();
    }

    std::vector<float> decodeSamplePCM(const juce::String& base64)
    {
        if (base64.isEmpty()) return {};
        juce::MemoryOutputStream mo;
        if (! juce::Base64::convertFromBase64(mo, base64)) return {};
        const auto* src = static_cast<const int16_t*>(mo.getData());
        const size_t n  = mo.getDataSize() / sizeof(int16_t);
        std::vector<float> out(n);
        for (size_t i = 0; i < n; ++i)
            out[i] = static_cast<float>(src[i]) / 32767.0f;
        return out;
    }
}

bool SampleSetIO::saveToFile(const SamplerConfig& sm, const juce::File& dst)
{
    juce::XmlElement root("DoomVizSampleSet");
    root.setAttribute("version", 1);

    for (int i = 0; i < kNumPads; ++i)
    {
        const auto& pad = sm.pads[i];
        auto* p = root.createNewChildElement("Pad");
        p->setAttribute("index",            i);
        p->setAttribute("name",             pad.name);
        p->setAttribute("sourceSampleRate", pad.sourceSampleRate);
        p->setAttribute("startSample",      pad.startSample);
        p->setAttribute("endSample",        pad.endSample);

        auto* pcm = p->createNewChildElement("SamplePCM");
        if (! pad.sampleData.empty())
            pcm->addTextElement(encodeSamplePCM(pad.sampleData));
    }

    return root.writeTo(dst);
}

std::optional<SamplerConfig> SampleSetIO::loadFromFile(const juce::File& src,
                                                         double hostSampleRate)
{
    if (! src.existsAsFile()) return std::nullopt;
    auto xml = juce::XmlDocument::parse(src);
    if (! xml || ! xml->hasTagName("DoomVizSampleSet")) return std::nullopt;

    SamplerConfig sm = SamplerConfig::makeDefault();

    for (auto* p : xml->getChildIterator())
    {
        if (! p->hasTagName("Pad")) continue;
        const int idx = p->getIntAttribute("index", -1);
        if (idx < 0 || idx >= kNumPads) continue;

        auto& pad = sm.pads[idx];
        pad.name        = p->getStringAttribute("name", pad.name);
        const double srcSr = p->getDoubleAttribute("sourceSampleRate", hostSampleRate);
        pad.startSample = p->getIntAttribute("startSample", 0);
        pad.endSample   = p->getIntAttribute("endSample",   0);

        if (auto* pcmXml = p->getChildByName("SamplePCM"))
        {
            auto raw = decodeSamplePCM(pcmXml->getAllSubText().trim());
            if (raw.empty()) { pad = PadConfig::makeDefault(); continue; }

            const bool needsResample = std::abs(srcSr - hostSampleRate) >= 0.5;
            if (needsResample && srcSr > 0.0 && hostSampleRate > 0.0)
            {
                const double scale = hostSampleRate / srcSr;
                auto resampled = resampleMono(raw, srcSr, hostSampleRate);
                const int n = static_cast<int>(resampled.size());

                pad.startSample = juce::jlimit(0, n,
                    static_cast<int>(pad.startSample * scale));
                pad.endSample   = juce::jlimit(pad.startSample, n,
                    pad.endSample > 0 ? static_cast<int>(pad.endSample * scale) : n);
                pad.sampleData       = std::move(resampled);
                pad.sourceSampleRate = hostSampleRate;
            }
            else
            {
                pad.sampleData       = std::move(raw);
                pad.sourceSampleRate = hostSampleRate;
                if (pad.endSample <= 0)
                    pad.endSample = static_cast<int>(pad.sampleData.size());
            }
        }
    }

    return sm;
}

} // namespace patch
