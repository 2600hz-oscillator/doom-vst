#include "VisualizerState.h"

namespace patch
{

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

juce::String VisualizerState::toXmlString() const
{
    GlobalConfig   g = getGlobal();
    SpectrumConfig s = getSpectrum();

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
    specXml->setAttribute("vibe", static_cast<int>(s.vibe));

    // KillRoom and Analyzer placeholders for forward-compat XML schema.
    root.createNewChildElement("KillRoom");
    root.createNewChildElement("Analyzer");

    return root.toString();
}

void VisualizerState::fromXmlString(const juce::String& xmlString)
{
    auto xml = juce::XmlDocument::parse(xmlString);
    if (xml == nullptr) return;

    GlobalConfig   g = GlobalConfig::makeDefault();
    SpectrumConfig s = SpectrumConfig::makeDefault();

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

    if (current)
    {
        if (auto* globalXml = root->getChildByName("Global"))
            readBands(globalXml);
        if (auto* specXml = root->getChildByName("Spectrum"))
        {
            int v = specXml->getIntAttribute("vibe", static_cast<int>(s.vibe));
            if (v >= 0 && v < kNumBackgroundVibes)
                s.vibe = static_cast<BackgroundVibe>(v);
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
}

} // namespace patch
