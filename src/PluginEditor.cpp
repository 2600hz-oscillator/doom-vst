#include "PluginEditor.h"

DoomVizEditor::DoomVizEditor(DoomVizProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      viewport(p.getSignalBus(), p.getCurrentSampleRate())
{
    addAndMakeVisible(viewport);

    setSize(960, 600);
    setResizable(true, true);
    setResizeLimits(320, 200, 1920, 1200);
}

DoomVizEditor::~DoomVizEditor() = default;

void DoomVizEditor::resized()
{
    viewport.setBounds(getLocalBounds());
}
