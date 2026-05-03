#include "SamplerPanel.h"

namespace patch
{

namespace
{
    constexpr auto kStoneDark = juce::uint32 { 0xff2a1f15 };
    constexpr auto kBevelHi   = juce::uint32 { 0xffc8b070 };
    constexpr auto kDimGold   = juce::uint32 { 0xff786850 };
    constexpr auto kGoldText  = juce::uint32 { 0xfff0d878 };
}

SamplerPanel::SamplerPanel(VisualizerState& s)
    : state(s)
{
    header.setColour(juce::Label::textColourId, juce::Colour(kBevelHi));
    header.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::bold));
    header.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(header);

    status.setColour(juce::Label::textColourId, juce::Colour(kDimGold));
    status.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::italic));
    status.setJustificationType(juce::Justification::centred);
    status.setText("9 pads, MIDI ch 1-9 → trigger. Pad UI lands in Phase 6.",
                   juce::dontSendNotification);
    addAndMakeVisible(status);
}

void SamplerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kStoneDark));

    // Mock 3×3 pad grid so the user can see where pads will live.
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(28);  // header
    area.removeFromTop(20);  // status line
    area.removeFromTop(8);

    const int rows = 3;
    const int cols = 3;
    const int gap  = 6;
    const int cellW = (area.getWidth()  - (cols - 1) * gap) / cols;
    const int cellH = (area.getHeight() - (rows - 1) * gap) / rows;

    g.setFont(juce::FontOptions("Courier New", 11.0f, juce::Font::bold));
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            const int padIdx = r * cols + c + 1;
            juce::Rectangle<int> cell(area.getX() + c * (cellW + gap),
                                       area.getY() + r * (cellH + gap),
                                       cellW, cellH);
            g.setColour(juce::Colour(0xff1a120a));
            g.fillRect(cell);
            g.setColour(juce::Colour(kDimGold));
            g.drawRect(cell, 1);
            g.setColour(juce::Colour(kGoldText));
            g.drawText("PAD " + juce::String(padIdx), cell,
                       juce::Justification::centred, false);
        }
    }
}

void SamplerPanel::resized()
{
    auto b = getLocalBounds().reduced(8);
    header.setBounds(b.removeFromTop(28));
    status.setBounds(b.removeFromTop(20));
}

} // namespace patch
