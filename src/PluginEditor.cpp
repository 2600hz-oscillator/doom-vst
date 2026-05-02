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
               p.getVisualizerState(), &p.sceneOverride, &p.currentSceneIndex)
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

    // Create the floating control window (single-window UI; patch settings
    // live inline rather than in a separate floating window).
    controlWindow = std::make_unique<ControlWindow>(p.getVisualizerState());

    controlWindow->getPanel().onSceneChange = [&p](int scene)
    {
        p.sceneOverride.store(scene, std::memory_order_relaxed);
    };
    controlWindow->getPanel().onToggleFullscreen = [this](bool on)
    {
        if (on)
            enterFullscreen();
        else
            exitFullscreen();
    };

    setSize(960, 600);
    setResizable(true, true);
    setResizeLimits(320, 200, 1920, 1200);

    // Push the initial display name immediately, then poll a few times a
    // second so the label tracks the editor as the user drags the host
    // window between monitors.
    timerCallback();
    startTimerHz(4);
}

DoomVizEditor::~DoomVizEditor()
{
    stopTimer();
    if (fullscreenWindow)
        exitFullscreen();
    controlWindow.reset();
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

    // Pick whichever display contains the most of the editor right now —
    // fullscreen lands on the monitor the user dragged the plugin window
    // onto, not always the primary display. Captured before reparenting
    // since reparenting changes the viewport's screen bounds.
    auto& displays = juce::Desktop::getInstance().getDisplays();
    const auto* targetDisplay = displays.getDisplayForRect(getScreenBounds());

    auto window = std::make_unique<FullscreenViewportWindow>();
    window->onClose = [this]() { exitFullscreen(); };

    // Reparent the viewport into the new top-level window.
    removeChildComponent(&viewport);
    window->setContentNonOwned(&viewport, false);

    // Position the window on the chosen display before showing it. macOS's
    // [NSWindow screen] then returns the right NSScreen, and the fallback
    // setKioskModeComponent path on other OSes uses the component's own
    // current display.
    if (targetDisplay != nullptr)
        window->setBounds(targetDisplay->totalArea);

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

void DoomVizEditor::timerCallback()
{
    // Display-name preview (where Fullscreen will land).
    auto name = describeTargetDisplay();
    if (name != lastDisplayName)
    {
        lastDisplayName = name;
        if (controlWindow)
            controlWindow->getPanel().setDisplayName(name);
    }

    // Scene-change reflection: when MIDI Program Change (or anything else that
    // bypasses the GUI button) switches the scene, the viewport publishes the
    // new index into processorRef.currentSceneIndex. Mirror that back into
    // the control panel so the active section matches what's on screen.
    int scene = processorRef.currentSceneIndex.load(std::memory_order_relaxed);
    if (scene != lastObservedScene)
    {
        lastObservedScene = scene;
        if (controlWindow)
            controlWindow->getPanel().setActiveScene(scene);
    }
}

juce::String DoomVizEditor::describeTargetDisplay() const
{
    const auto& displays = juce::Desktop::getInstance().getDisplays();
    if (displays.displays.isEmpty())
        return "-";

    auto bounds = getScreenBounds();
    const auto* d = displays.getDisplayForRect(bounds);
    if (d == nullptr)
        return "-";

    int idx = 0;
    for (int i = 0; i < displays.displays.size(); ++i)
        if (&displays.displays.getReference(i) == d) { idx = i; break; }

    auto w = d->totalArea.getWidth();
    auto h = d->totalArea.getHeight();
    juce::String prefix = d->isMain ? juce::String("Main")
                                    : juce::String("Display ") + juce::String(idx + 1);
    return prefix + ", " + juce::String(w) + "x" + juce::String(h);
}
