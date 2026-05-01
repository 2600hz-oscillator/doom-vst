#pragma once

#include "PatchSettings.h"
#include <juce_core/juce_core.h>

namespace patch
{

// Holds patch settings for each scene. GUI thread writes (Apply button);
// render thread reads (per-frame snapshot copy under SpinLock). The lock is
// only contended during a click, so the render-thread copy is essentially free.
class PatchSettingsStore
{
public:
    PatchSettingsStore();

    SpectrumSettings getSpectrum() const;
    void setSpectrum(const SpectrumSettings& s);

    // Serialize/restore as XML (for AudioProcessor state save/restore).
    juce::String toXmlString() const;
    void fromXmlString(const juce::String& xml);

private:
    mutable juce::SpinLock lock;
    SpectrumSettings spectrum { SpectrumSettings::makeDefault() };
};

} // namespace patch
