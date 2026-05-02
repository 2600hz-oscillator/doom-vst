#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace patch
{

// Sub-section shown for scenes that have no editable per-scene parameters
// yet (KillRoom, Analyzer until later phases). Renders a single muted
// "no editable parameters yet" line.
class PlaceholderSection : public juce::Component
{
public:
    explicit PlaceholderSection(const juce::String& sceneName);

    void paint(juce::Graphics&) override;

private:
    juce::String name;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaceholderSection)
};

} // namespace patch
