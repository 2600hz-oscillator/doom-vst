#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace patch
{

// Used for scenes that don't yet have editable patch parameters (Kill Room,
// Analyzer Room). Shows a centered "no editable parameters yet" message.
class PlaceholderPatchPanel : public juce::Component
{
public:
    explicit PlaceholderPatchPanel(const juce::String& sceneName);

    void paint(juce::Graphics&) override;

private:
    juce::String name;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaceholderPatchPanel)
};

} // namespace patch
