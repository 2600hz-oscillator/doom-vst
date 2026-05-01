#include "PlaceholderPatchPanel.h"

namespace patch
{

PlaceholderPatchPanel::PlaceholderPatchPanel(const juce::String& sceneName)
    : name(sceneName)
{
    setSize(480, 340);
}

void PlaceholderPatchPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    g.setColour(juce::Colour(0xff888888));
    g.setFont(juce::FontOptions(14.0f));

    auto area = getLocalBounds().reduced(20);
    g.drawFittedText(name + ": no editable parameters yet.",
                     area, juce::Justification::centred, 2);
}

} // namespace patch
