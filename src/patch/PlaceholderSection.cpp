#include "PlaceholderSection.h"

namespace patch
{

namespace
{
    constexpr auto kStoneDark = juce::uint32 { 0xff2a1f15 };
    constexpr auto kDimGold   = juce::uint32 { 0xff786850 };
}

PlaceholderSection::PlaceholderSection(const juce::String& sceneName)
    : name(sceneName)
{
}

void PlaceholderSection::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kStoneDark));
    g.setColour(juce::Colour(kDimGold));
    g.setFont(juce::FontOptions("Courier New", 12.0f, juce::Font::italic));
    g.drawFittedText(name + ": no editable parameters yet.",
                     getLocalBounds(), juce::Justification::centred, 1);
}

} // namespace patch
