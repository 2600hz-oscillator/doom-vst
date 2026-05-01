#include "PatchSettingsStore.h"

namespace patch
{

PatchSettingsStore::PatchSettingsStore() = default;

SpectrumSettings PatchSettingsStore::getSpectrum() const
{
    const juce::SpinLock::ScopedLockType l(lock);
    return spectrum;
}

void PatchSettingsStore::setSpectrum(const SpectrumSettings& s)
{
    const juce::SpinLock::ScopedLockType l(lock);
    spectrum = s;
}

juce::String PatchSettingsStore::toXmlString() const
{
    SpectrumSettings snap = getSpectrum();

    juce::XmlElement root("DoomVizPatches");
    auto* spec = root.createNewChildElement("Spectrum");
    for (int i = 0; i < kSpectrumNumBands; ++i)
    {
        auto* b = spec->createNewChildElement("Band");
        b->setAttribute("index", i);
        b->setAttribute("lowHz",    static_cast<double>(snap.bands[i].lowHz));
        b->setAttribute("highHz",   static_cast<double>(snap.bands[i].highHz));
        b->setAttribute("gain01",   static_cast<double>(snap.bands[i].gain01));
        b->setAttribute("spriteId", snap.bands[i].spriteId);
    }
    return root.toString();
}

void PatchSettingsStore::fromXmlString(const juce::String& xmlString)
{
    auto xml = juce::XmlDocument::parse(xmlString);
    if (xml == nullptr || ! xml->hasTagName("DoomVizPatches"))
        return;

    SpectrumSettings s = SpectrumSettings::makeDefault();
    if (auto* spec = xml->getChildByName("Spectrum"))
    {
        for (auto* b : spec->getChildIterator())
        {
            if (! b->hasTagName("Band")) continue;
            int idx = b->getIntAttribute("index", -1);
            if (idx < 0 || idx >= kSpectrumNumBands) continue;
            s.bands[idx].lowHz    = static_cast<float>(b->getDoubleAttribute("lowHz",  s.bands[idx].lowHz));
            s.bands[idx].highHz   = static_cast<float>(b->getDoubleAttribute("highHz", s.bands[idx].highHz));
            s.bands[idx].gain01   = static_cast<float>(b->getDoubleAttribute("gain01", s.bands[idx].gain01));
            s.bands[idx].spriteId = b->getIntAttribute("spriteId", s.bands[idx].spriteId);
        }
    }
    setSpectrum(s);
}

} // namespace patch
