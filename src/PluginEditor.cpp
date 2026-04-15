#include "PluginEditor.h"

DoomVizEditor::DoomVizEditor(DoomVizProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      viewport(p.getSignalBus(), p.getCurrentSampleRate(), &p.sceneOverride)
{
    addAndMakeVisible(viewport);

    // Create the floating control window
    controlWindow = std::make_unique<ControlWindow>();
    controlWindow->getPanel().onSceneChange = [&p](int scene)
    {
        p.sceneOverride.store(scene, std::memory_order_relaxed);
    };

    setSize(960, 600);
    setResizable(true, true);
    setResizeLimits(320, 200, 1920, 1200);
}

DoomVizEditor::~DoomVizEditor()
{
    controlWindow.reset();
}

void DoomVizEditor::resized()
{
    viewport.setBounds(getLocalBounds());
}
