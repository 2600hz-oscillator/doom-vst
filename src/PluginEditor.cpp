#include "PluginEditor.h"

#if JUCE_MAC
extern void macEnterTrueFullscreen(juce::Component&);
extern void macExitTrueFullscreen(juce::Component&);
#endif

namespace
{
    // Borderless top-level window used to host the viewport in fullscreen mode.
    class FullscreenViewportWindow : public juce::DocumentWindow
    {
    public:
        FullscreenViewportWindow()
            : DocumentWindow("DoomViz", juce::Colours::black, 0)
        {
            setUsingNativeTitleBar(false);
            setTitleBarHeight(0);
            setDropShadowEnabled(false);
        }

        std::function<void()> onClose;
        void closeButtonPressed() override { if (onClose) onClose(); }
    };
}

DoomVizEditor::DoomVizEditor(DoomVizProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      viewport(p.getSignalBus(), p.getCurrentSampleRate(),
               p.getPatchSettings(), &p.sceneOverride)
{
    addAndMakeVisible(viewport);

    // Double-clicking the viewport while in fullscreen exits — provides an
    // escape hatch in case the floating control window is hidden behind it.
    // Defer via callAsync so the mouse event finishes unwinding before we
    // tear down the window that owns it (otherwise the OS event loop can
    // wedge with a phantom window stuck at CGShieldingWindowLevel).
    viewport.onDoubleClick = [this]()
    {
        if (! fullscreenWindow) return;
        juce::MessageManager::callAsync([this]()
        {
            if (fullscreenWindow)
                exitFullscreen();
        });
    };

    // Create the floating control window
    controlWindow = std::make_unique<ControlWindow>();

    // Create the patch settings window (hidden by default; opened via the
    // Patch Settings button on the control panel).
    patchWindow = std::make_unique<patch::PatchWindow>(p.getPatchSettings());

    controlWindow->getPanel().onSceneChange = [this, &p](int scene)
    {
        p.sceneOverride.store(scene, std::memory_order_relaxed);
        if (patchWindow)
            patchWindow->setActiveScene(scene);
    };
    controlWindow->getPanel().onToggleFullscreen = [this](bool on)
    {
        if (on)
            enterFullscreen();
        else
            exitFullscreen();
    };
    controlWindow->getPanel().onTogglePatchSettings = [this]()
    {
        togglePatchWindow();
    };

    setSize(960, 600);
    setResizable(true, true);
    setResizeLimits(320, 200, 1920, 1200);
}

DoomVizEditor::~DoomVizEditor()
{
    if (fullscreenWindow)
        exitFullscreen();
    patchWindow.reset();
    controlWindow.reset();
}

void DoomVizEditor::togglePatchWindow()
{
    if (! patchWindow) return;
    bool nowVisible = ! patchWindow->isVisible();
    patchWindow->setVisible(nowVisible);
    if (nowVisible)
        patchWindow->toFront(true);
}

void DoomVizEditor::resized()
{
    if (! fullscreenWindow)
        viewport.setBounds(getLocalBounds());
}

void DoomVizEditor::enterFullscreen()
{
    if (fullscreenWindow)
        return;

    auto window = std::make_unique<FullscreenViewportWindow>();
    window->onClose = [this]() { exitFullscreen(); };

    // Reparent the viewport into the new top-level window.
    removeChildComponent(&viewport);
    window->setContentNonOwned(&viewport, false);
    window->setVisible(true);

    fullscreenWindow = std::move(window);

#if JUCE_MAC
    // Native Cocoa: elevate the window above the menu bar and hide menu/dock.
    macEnterTrueFullscreen(*fullscreenWindow);
#else
    juce::Desktop::getInstance().setKioskModeComponent(fullscreenWindow.get(), false);
#endif
}

void DoomVizEditor::exitFullscreen()
{
    if (! fullscreenWindow)
        return;

#if JUCE_MAC
    macExitTrueFullscreen(*fullscreenWindow);
#else
    juce::Desktop::getInstance().setKioskModeComponent(nullptr);
#endif

    fullscreenWindow->clearContentComponent();
    fullscreenWindow.reset();

    addAndMakeVisible(viewport);
    viewport.setBounds(getLocalBounds());

    // Restore key-window status to the editor's host window so Bitwig
    // (or whichever DAW) regains keyboard / mouse focus.
    if (auto* peer = getPeer())
        peer->grabFocus();

    // Sync the control panel button label/state back to "Fullscreen" — the
    // user may have exited via double-click instead of the button.
    if (controlWindow)
        controlWindow->getPanel().setFullscreenState(false);
}
