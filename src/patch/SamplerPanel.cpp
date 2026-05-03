#include "SamplerPanel.h"
#include "SampleSetIO.h"

#include <algorithm>
#include <cmath>

namespace patch
{

namespace
{
    constexpr auto kStoneDark  = juce::uint32 { 0xff2a1f15 };
    constexpr auto kStoneLight = juce::uint32 { 0xff3a2f25 };
    constexpr auto kBevelHi    = juce::uint32 { 0xffc8b070 };
    constexpr auto kDimGold    = juce::uint32 { 0xff786850 };
    constexpr auto kGoldText   = juce::uint32 { 0xfff0d878 };

    // Cap a pad at 30 sec. Plan calls this out as a v1 safeguard so an
    // accidental 30-min WAV doesn't blow up project state.
    constexpr double kMaxSampleSeconds = 30.0;

    std::vector<float> decodeWav(const juce::File& file,
                                  double hostSampleRate,
                                  juce::AudioFormatManager& fm)
    {
        if (! file.existsAsFile()) return {};

        std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(file));
        if (! reader) return {};

        const double srcSr = reader->sampleRate;
        const int    numCh = static_cast<int>(reader->numChannels);
        if (srcSr <= 0.0 || numCh <= 0) return {};

        const int srcLen   = static_cast<int>(reader->lengthInSamples);
        if (srcLen <= 0) return {};

        const int srcCap   = static_cast<int>(srcSr * kMaxSampleSeconds);
        const int readLen  = std::min(srcLen, srcCap);

        juce::AudioBuffer<float> srcBuf(numCh, readLen);
        if (! reader->read(&srcBuf, 0, readLen, 0, true, true)) return {};

        // Mono downmix.
        std::vector<float> mono(static_cast<size_t>(readLen), 0.0f);
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* p = srcBuf.getReadPointer(ch);
            for (int i = 0; i < readLen; ++i)
                mono[static_cast<size_t>(i)] += p[i];
        }
        if (numCh > 1)
            for (auto& s : mono) s /= static_cast<float>(numCh);

        // No-op if the WAV is already at the host SR.
        if (std::abs(srcSr - hostSampleRate) < 0.5)
            return mono;

        // juce::Interpolators::Lagrange3rd uses speedRatio = src/dst:
        // ratio < 1 upsamples (longer output), ratio > 1 downsamples.
        const double speedRatio = srcSr / hostSampleRate;
        const int    dstLen     = static_cast<int>(std::ceil(readLen / speedRatio));
        std::vector<float> dst(static_cast<size_t>(dstLen), 0.0f);

        juce::Interpolators::Lagrange interp;
        interp.process(speedRatio, mono.data(), dst.data(), dstLen);
        return dst;
    }
}

SamplerPanel::SamplerPanel(VisualizerState& s,
                            std::function<double()> getSr,
                            std::function<void(int)> trig)
    : state(s),
      getHostSampleRate(std::move(getSr)),
      triggerPad(std::move(trig))
{
    formatManager.registerBasicFormats();

    header.setColour(juce::Label::textColourId, juce::Colour(kBevelHi));
    header.setFont(juce::FontOptions("Courier New", 13.0f, juce::Font::bold));
    header.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(header);

    hint.setColour(juce::Label::textColourId, juce::Colour(kDimGold));
    hint.setFont(juce::FontOptions("Courier New", 10.0f, juce::Font::italic));
    hint.setJustificationType(juce::Justification::centredLeft);
    hint.setText("MIDI ch 1-9 → pads 1-9. Root C (MIDI 60) plays at 1.0x.",
                 juce::dontSendNotification);
    addAndMakeVisible(hint);

    auto styleSetButton = [](juce::TextButton& btn)
    {
        btn.setColour(juce::TextButton::buttonColourId,    juce::Colour(kStoneLight));
        btn.setColour(juce::TextButton::textColourOffId,   juce::Colour(kGoldText));
    };
    styleSetButton(saveSetBtn);
    styleSetButton(loadSetBtn);
    saveSetBtn.onClick = [this]() { handleSaveSet(); };
    loadSetBtn.onClick = [this]() { handleLoadSet(); };
    addAndMakeVisible(saveSetBtn);
    addAndMakeVisible(loadSetBtn);

    for (int i = 0; i < kNumPads; ++i)
    {
        cells[i].onLoadClicked    = [this, i]() { handleLoadPad(i); };
        cells[i].onTriggerClicked = [this, i]() { if (triggerPad) triggerPad(i); };
        cells[i].onMarkersChanged = [this, i](int s, int e)
        {
            // Marker-only update: cheap SpinLock-protected int writes,
            // no full-config copy. Audio thread picks it up on next
            // processMidi() snapshot.
            state.setPadMarkers(i, s, e);
        };
        addAndMakeVisible(cells[i]);
    }

    loadFromState(state.getSampler());
}

void SamplerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(kStoneDark));
}

void SamplerPanel::resized()
{
    auto b = getLocalBounds().reduced(8);
    header.setBounds(b.removeFromTop(20));
    b.removeFromTop(2);
    hint  .setBounds(b.removeFromTop(16));
    b.removeFromTop(6);

    // SAVE SET / LOAD SET row above the pad grid.
    auto setRow = b.removeFromTop(22);
    const int half = setRow.getWidth() / 2;
    saveSetBtn.setBounds(setRow.removeFromLeft(half - 2));
    setRow.removeFromLeft(4);
    loadSetBtn.setBounds(setRow);
    b.removeFromTop(8);

    const int rows = 3;
    const int cols = 3;
    const int gap  = 6;
    const int cellW = (b.getWidth()  - (cols - 1) * gap) / cols;
    const int cellH = (b.getHeight() - (rows - 1) * gap) / rows;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            const int idx = r * cols + c;
            cells[idx].setBounds(b.getX() + c * (cellW + gap),
                                  b.getY() + r * (cellH + gap),
                                  cellW, cellH);
        }
    }
}

void SamplerPanel::loadFromState(const SamplerConfig& sm)
{
    for (int i = 0; i < kNumPads; ++i)
        cells[i].setPad(sm.pads[i]);
}

void SamplerPanel::handleLoadPad(int padIndex)
{
    if (padIndex < 0 || padIndex >= kNumPads) return;

    auto initialDir = findBundledSoundsDir();
    if (initialDir == juce::File{})
        initialDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample for Pad " + juce::String(padIndex + 1),
        initialDir,
        "*.wav;*.aif;*.aiff",
        true);

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, padIndex](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File{}) return; // user cancelled

        const double hostSr = getHostSampleRate ? getHostSampleRate() : 44100.0;
        auto data = decodeWav(file, hostSr, formatManager);
        if (data.empty()) return; // unreadable / malformed; leave pad untouched

        commitPad(padIndex, file.getFileName(), hostSr, std::move(data));
    });
}

void SamplerPanel::commitPad(int padIndex, juce::String name,
                              double srcSampleRate, std::vector<float> data)
{
    auto cfg = state.getSampler();
    auto& pad = cfg.pads[padIndex];
    pad.name             = std::move(name);
    pad.sourceSampleRate = srcSampleRate;
    pad.startSample      = 0;
    pad.endSample        = static_cast<int>(data.size());
    pad.sampleData       = std::move(data);

    state.setSampler(cfg);
    cells[padIndex].setPad(pad);
}

void SamplerPanel::handleSaveSet()
{
    auto initialDir = userSetsDir();
    initialDir.createDirectory();   // ensure ~/Documents/DoomViz Sets exists

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Sample Set",
        initialDir.getChildFile("Untitled.dvset"),
        "*.dvset",
        true);

    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{}) return;
        if (file.getFileExtension().isEmpty())
            file = file.withFileExtension(".dvset");

        SampleSetIO::saveToFile(state.getSampler(), file);
    });
}

void SamplerPanel::handleLoadSet()
{
    auto initialDir = userSetsDir();
    if (! initialDir.isDirectory())
    {
        // Fall back to the bundled sets directory if the user hasn't
        // saved anything yet.
        auto thisModule = juce::File::getSpecialLocation(
            juce::File::currentExecutableFile);
        auto bundleSets = thisModule.getParentDirectory()
                                    .getParentDirectory()
                                    .getParentDirectory()
                                    .getChildFile("Contents/Resources/sets");
        if (bundleSets.isDirectory()) initialDir = bundleSets;
        else initialDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Sample Set",
        initialDir,
        "*.dvset",
        true);

    fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File{}) return;

        const double hostSr = getHostSampleRate ? getHostSampleRate() : 44100.0;
        if (auto loaded = SampleSetIO::loadFromFile(file, hostSr))
        {
            state.setSampler(*loaded);
            loadFromState(*loaded);
        }
    });
}

juce::File SamplerPanel::userSetsDir()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
               .getChildFile("DoomViz Sets");
}

juce::File SamplerPanel::findBundledSoundsDir()
{
    auto thisModule = juce::File::getSpecialLocation(
        juce::File::currentExecutableFile);
    auto bundleRoot = thisModule.getParentDirectory()
                                .getParentDirectory()
                                .getParentDirectory();

    auto bundleResources = bundleRoot.getChildFile("Contents/Resources/sounds");
    if (bundleResources.isDirectory()) return bundleResources;

    auto nextToBundle = bundleRoot.getParentDirectory().getChildFile("sounds");
    if (nextToBundle.isDirectory()) return nextToBundle;

    auto devPath = juce::File::getCurrentWorkingDirectory()
                       .getChildFile("resources/sounds");
    if (devPath.isDirectory()) return devPath;

    return {};
}

} // namespace patch
