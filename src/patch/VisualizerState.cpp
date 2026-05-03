#include "VisualizerState.h"

#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>

namespace patch
{

namespace
{
    // Float buffer ↔ base64-encoded 16-bit signed little-endian PCM.
    // 16-bit is plenty for sampler audio and halves the XML state size
    // vs. raw 32-bit floats.
    juce::String encodeSamplePCM(const std::vector<float>& samples)
    {
        if (samples.empty()) return {};
        juce::MemoryBlock pcm;
        pcm.setSize(samples.size() * sizeof(int16_t));
        auto* dst = static_cast<int16_t*>(pcm.getData());
        for (size_t i = 0; i < samples.size(); ++i)
        {
            float s = juce::jlimit(-1.0f, 1.0f, samples[i]);
            dst[i] = static_cast<int16_t>(std::lround(s * 32767.0f));
        }
        return pcm.toBase64Encoding();
    }

    std::vector<float> decodeSamplePCM(const juce::String& base64)
    {
        if (base64.isEmpty()) return {};
        juce::MemoryBlock pcm;
        if (! pcm.fromBase64Encoding(base64)) return {};
        const size_t n = pcm.getSize() / sizeof(int16_t);
        const auto* src = static_cast<const int16_t*>(pcm.getData());
        std::vector<float> out(n);
        for (size_t i = 0; i < n; ++i)
            out[i] = static_cast<float>(src[i]) / 32767.0f;
        return out;
    }
}


VisualizerState::VisualizerState() = default;

GlobalConfig VisualizerState::getGlobal() const
{
    const juce::SpinLock::ScopedLockType l(lock);
    return global;
}

void VisualizerState::setGlobal(const GlobalConfig& g)
{
    const juce::SpinLock::ScopedLockType l(lock);
    global = g;
}

SpectrumConfig VisualizerState::getSpectrum() const
{
    const juce::SpinLock::ScopedLockType l(lock);
    return spectrum;
}

void VisualizerState::setSpectrum(const SpectrumConfig& s)
{
    const juce::SpinLock::ScopedLockType l(lock);
    spectrum = s;
}

KillRoomConfig VisualizerState::getKillRoom() const
{
    const juce::SpinLock::ScopedLockType l(lock);
    return killRoom;
}

void VisualizerState::setKillRoom(const KillRoomConfig& k)
{
    const juce::SpinLock::ScopedLockType l(lock);
    killRoom = k;
}

AnalyzerConfig VisualizerState::getAnalyzer() const
{
    const juce::SpinLock::ScopedLockType l(lock);
    return analyzer;
}

void VisualizerState::setAnalyzer(const AnalyzerConfig& a)
{
    const juce::SpinLock::ScopedLockType l(lock);
    analyzer = a;
}

SamplerConfig VisualizerState::getSampler() const
{
    const juce::SpinLock::ScopedLockType l(lock);
    return sampler;
}

void VisualizerState::setSampler(const SamplerConfig& s)
{
    const juce::SpinLock::ScopedLockType l(lock);
    sampler = s;
}

juce::String VisualizerState::toXmlString() const
{
    GlobalConfig   g = getGlobal();
    SpectrumConfig s = getSpectrum();
    SamplerConfig  sm = getSampler();

    juce::XmlElement root("DoomVizState");

    auto* globalXml = root.createNewChildElement("Global");
    for (int i = 0; i < kNumBands; ++i)
    {
        auto* b = globalXml->createNewChildElement("Band");
        b->setAttribute("index",    i);
        b->setAttribute("lowHz",    static_cast<double>(g.bands[i].lowHz));
        b->setAttribute("highHz",   static_cast<double>(g.bands[i].highHz));
        b->setAttribute("gain01",   static_cast<double>(g.bands[i].gain01));
        b->setAttribute("spriteId", g.bands[i].spriteId);
    }

    auto* specXml = root.createNewChildElement("Spectrum");
    specXml->setAttribute("vibe",                 static_cast<int>(s.vibe));
    specXml->setAttribute("doomtexIndex",         s.doomtexIndex);
    specXml->setAttribute("doomtexAutoAdvance",   s.doomtexAutoAdvance);
    specXml->setAttribute("doomtexAutoBand",      s.doomtexAutoBand);
    specXml->setAttribute("doomtexAutoThreshold", static_cast<double>(s.doomtexAutoThreshold));

    // KillRoom and Analyzer placeholders for forward-compat XML schema.
    root.createNewChildElement("KillRoom");
    root.createNewChildElement("Analyzer");

    auto* samplerXml = root.createNewChildElement("Sampler");
    for (int i = 0; i < kNumPads; ++i)
    {
        const auto& pad = sm.pads[i];
        auto* padXml = samplerXml->createNewChildElement("Pad");
        padXml->setAttribute("index",            i);
        padXml->setAttribute("name",             pad.name);
        padXml->setAttribute("sourceSampleRate", pad.sourceSampleRate);
        padXml->setAttribute("startSample",      pad.startSample);
        padXml->setAttribute("endSample",        pad.endSample);

        auto* pcmXml = padXml->createNewChildElement("SamplePCM");
        if (! pad.sampleData.empty())
            pcmXml->addTextElement(encodeSamplePCM(pad.sampleData));
    }

    return root.toString();
}

void VisualizerState::fromXmlString(const juce::String& xmlString)
{
    auto xml = juce::XmlDocument::parse(xmlString);
    if (xml == nullptr) return;

    GlobalConfig   g  = GlobalConfig::makeDefault();
    SpectrumConfig s  = SpectrumConfig::makeDefault();
    SamplerConfig  sm = SamplerConfig::makeDefault();

    // Accept both the new schema (DoomVizState) and the legacy schema
    // (DoomVizPatches with bands inside <Spectrum>) so projects saved
    // before this rename keep loading.
    juce::XmlElement* root = xml.get();
    const bool legacy = root->hasTagName("DoomVizPatches");
    const bool current = root->hasTagName("DoomVizState");
    if (! legacy && ! current) return;

    auto readBands = [&](juce::XmlElement* parent)
    {
        for (auto* b : parent->getChildIterator())
        {
            if (! b->hasTagName("Band")) continue;
            int idx = b->getIntAttribute("index", -1);
            if (idx < 0 || idx >= kNumBands) continue;
            g.bands[idx].lowHz    = static_cast<float>(b->getDoubleAttribute("lowHz",    g.bands[idx].lowHz));
            g.bands[idx].highHz   = static_cast<float>(b->getDoubleAttribute("highHz",   g.bands[idx].highHz));
            g.bands[idx].gain01   = static_cast<float>(b->getDoubleAttribute("gain01",   g.bands[idx].gain01));
            g.bands[idx].spriteId = b->getIntAttribute("spriteId",                       g.bands[idx].spriteId);
        }
    };

    auto readSpectrumExtras = [&](juce::XmlElement* specXml)
    {
        s.doomtexIndex         = juce::jlimit(0, 7,
            specXml->getIntAttribute("doomtexIndex", s.doomtexIndex));
        s.doomtexAutoAdvance   =
            specXml->getBoolAttribute("doomtexAutoAdvance", s.doomtexAutoAdvance);
        s.doomtexAutoBand      = juce::jlimit(1, kNumBands,
            specXml->getIntAttribute("doomtexAutoBand", s.doomtexAutoBand));
        s.doomtexAutoThreshold = juce::jlimit(0.0f, 1.0f, static_cast<float>(
            specXml->getDoubleAttribute("doomtexAutoThreshold", s.doomtexAutoThreshold)));
    };

    if (current)
    {
        if (auto* globalXml = root->getChildByName("Global"))
            readBands(globalXml);
        if (auto* specXml = root->getChildByName("Spectrum"))
        {
            int v = specXml->getIntAttribute("vibe", static_cast<int>(s.vibe));
            if (v >= 0 && v < kNumBackgroundVibes)
                s.vibe = static_cast<BackgroundVibe>(v);
            readSpectrumExtras(specXml);
        }
        if (auto* samplerXml = root->getChildByName("Sampler"))
        {
            for (auto* p : samplerXml->getChildIterator())
            {
                if (! p->hasTagName("Pad")) continue;
                int idx = p->getIntAttribute("index", -1);
                if (idx < 0 || idx >= kNumPads) continue;
                auto& pad = sm.pads[idx];
                pad.name             = p->getStringAttribute("name", pad.name);
                pad.sourceSampleRate = p->getDoubleAttribute("sourceSampleRate", pad.sourceSampleRate);
                pad.startSample      = p->getIntAttribute("startSample", pad.startSample);
                pad.endSample        = p->getIntAttribute("endSample",   pad.endSample);
                if (auto* pcmXml = p->getChildByName("SamplePCM"))
                    pad.sampleData = decodeSamplePCM(pcmXml->getAllSubText().trim());
            }
        }
    }
    else // legacy DoomVizPatches: bands lived inside <Spectrum>
    {
        if (auto* specXml = root->getChildByName("Spectrum"))
        {
            int v = specXml->getIntAttribute("vibe", static_cast<int>(s.vibe));
            if (v >= 0 && v < kNumBackgroundVibes)
                s.vibe = static_cast<BackgroundVibe>(v);
            readBands(specXml);
        }
    }

    setGlobal(g);
    setSpectrum(s);
    setSampler(sm);
}

} // namespace patch
