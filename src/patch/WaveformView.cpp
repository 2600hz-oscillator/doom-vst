#include "WaveformView.h"
#include <algorithm>

namespace patch
{

namespace
{
    constexpr auto kBgDark      = juce::uint32 { 0xff140a04 };
    constexpr auto kWaveColor   = juce::uint32 { 0xff8a7848 };
    constexpr auto kStartColor  = juce::uint32 { 0xffc8b070 };  // gold
    constexpr auto kEndColor    = juce::uint32 { 0xffe04040 };  // red
    constexpr auto kTrimColor   = juce::uint32 { 0x402a2010 };  // muted overlay

    // Hit-test radius around a handle bar (px on either side).
    constexpr int kHandleHitRadius = 6;
}

WaveformView::WaveformView()
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void WaveformView::setPad(const PadConfig& pad)
{
    sourceData   = pad.sampleData;
    totalSamples = static_cast<int>(sourceData.size());
    startSample  = juce::jlimit(0, totalSamples, pad.startSample);
    endSample    = juce::jlimit(startSample, totalSamples,
                                  pad.endSample > 0 ? pad.endSample : totalSamples);
    peaksDirty   = true;
    repaint();
}

void WaveformView::resized()
{
    peaksDirty = true;
}

int WaveformView::xToSample(int x) const
{
    if (totalSamples <= 0 || getWidth() <= 0) return 0;
    const double frac = juce::jlimit(0.0, 1.0,
                                       static_cast<double>(x) / getWidth());
    return static_cast<int>(frac * totalSamples);
}

int WaveformView::sampleToX(int sample) const
{
    if (totalSamples <= 0) return 0;
    return static_cast<int>(
        static_cast<double>(sample) / totalSamples * getWidth());
}

WaveformView::DragTarget WaveformView::hitTestHandle(int x) const
{
    if (totalSamples <= 0) return DragTarget::None;
    if (std::abs(x - sampleToX(startSample)) <= kHandleHitRadius)
        return DragTarget::Start;
    if (std::abs(x - sampleToX(endSample)) <= kHandleHitRadius)
        return DragTarget::End;
    return DragTarget::None;
}

void WaveformView::recomputePeaks()
{
    peaks.clear();
    peaksDirty = false;

    if (sourceData.empty() || getWidth() <= 0) return;

    const int width = getWidth();
    peaks.resize(static_cast<size_t>(width));

    const int n = static_cast<int>(sourceData.size());
    for (int x = 0; x < width; ++x)
    {
        const int s0 = static_cast<int>((static_cast<int64_t>(x)     * n) / width);
        const int s1 = static_cast<int>((static_cast<int64_t>(x + 1) * n) / width);
        const int hi = std::min(s1, n);
        if (s0 >= hi) { peaks[static_cast<size_t>(x)] = {0.0f, 0.0f}; continue; }

        float lo = sourceData[static_cast<size_t>(s0)];
        float up = lo;
        for (int i = s0 + 1; i < hi; ++i)
        {
            const float s = sourceData[static_cast<size_t>(i)];
            if (s < lo) lo = s;
            if (s > up) up = s;
        }
        peaks[static_cast<size_t>(x)] = {lo, up};
    }
}

void WaveformView::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();
    g.setColour(juce::Colour(kBgDark));
    g.fillRect(b);

    if (peaksDirty) recomputePeaks();

    if (peaks.empty())
    {
        g.setColour(juce::Colour(kWaveColor).withAlpha(0.3f));
        g.drawText("(no sample)", b, juce::Justification::centred, false);
        return;
    }

    const int height = b.getHeight();
    const int midY   = b.getCentreY();
    const float halfH = static_cast<float>(height) / 2.0f;

    // Waveform: vertical line per pixel column from min to max.
    g.setColour(juce::Colour(kWaveColor));
    for (size_t x = 0; x < peaks.size(); ++x)
    {
        const auto [lo, up] = peaks[x];
        const int y0 = midY - static_cast<int>(up * halfH);
        const int y1 = midY - static_cast<int>(lo * halfH);
        g.fillRect(static_cast<int>(x), std::min(y0, y1),
                   1, std::max(1, std::abs(y1 - y0)));
    }

    // Trim overlay: darken outside the [start, end] range.
    g.setColour(juce::Colour(kTrimColor));
    const int xs = sampleToX(startSample);
    const int xe = sampleToX(endSample);
    if (xs > 0)               g.fillRect(0, 0, xs, height);
    if (xe < b.getWidth())    g.fillRect(xe, 0, b.getWidth() - xe, height);

    // Handles on top.
    auto drawHandle = [&](int x, juce::Colour c, bool active)
    {
        g.setColour(c);
        const int thick = active ? 2 : 1;
        g.fillRect(juce::jlimit(0, b.getWidth() - thick, x), 0, thick, height);
        // Top + bottom pip so a flat-color handle still reads as a handle.
        g.fillRect(juce::jlimit(0, b.getWidth() - 4, x - 1), 0,        4, 3);
        g.fillRect(juce::jlimit(0, b.getWidth() - 4, x - 1), height-3, 4, 3);
    };
    drawHandle(xs, juce::Colour(kStartColor),
               dragging == DragTarget::Start || hovered == DragTarget::Start);
    drawHandle(xe, juce::Colour(kEndColor),
               dragging == DragTarget::End   || hovered == DragTarget::End);
}

void WaveformView::mouseDown(const juce::MouseEvent& e)
{
    dragging = hitTestHandle(e.x);
    if (dragging != DragTarget::None)
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void WaveformView::mouseDrag(const juce::MouseEvent& e)
{
    if (dragging == DragTarget::None) return;
    int s = juce::jlimit(0, totalSamples, xToSample(e.x));

    if (dragging == DragTarget::Start)
        startSample = std::min(s, std::max(0, endSample - 1));
    else
        endSample   = std::max(s, std::min(totalSamples, startSample + 1));

    repaint();
    notifyMarkersChanged();
}

void WaveformView::mouseUp(const juce::MouseEvent&)
{
    dragging = DragTarget::None;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void WaveformView::mouseMove(const juce::MouseEvent& e)
{
    auto h = hitTestHandle(e.x);
    if (h != hovered)
    {
        hovered = h;
        setMouseCursor(h == DragTarget::None
                       ? juce::MouseCursor::NormalCursor
                       : juce::MouseCursor::LeftRightResizeCursor);
        repaint();
    }
}

void WaveformView::notifyMarkersChanged()
{
    if (onMarkersChanged) onMarkersChanged(startSample, endSample);
}

} // namespace patch
