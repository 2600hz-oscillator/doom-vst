#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include "routing/SignalRouter.h"
#include "routing/RouteConfig.h"
#include "scenes/SceneManager.h"
#include "patch/PatchSettingsStore.h"
#include <functional>
#include <memory>

class DoomViewport : public juce::Component,
                     public juce::OpenGLRenderer
{
public:
    DoomViewport(SignalBus& signalBus, double sampleRate,
                 const patch::PatchSettingsStore& patchStore,
                 std::atomic<int>* sceneOverride = nullptr,
                 std::atomic<int>* currentSceneIndex = nullptr);
    ~DoomViewport() override;

    void setSampleRate(double sr);

    // OpenGLRenderer callbacks
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // Access analyzer results
    const AudioAnalyzer& getAnalyzer() const { return analyzer; }

    // Called on a double-click anywhere in the viewport. Used by the editor to
    // exit fullscreen mode.
    std::function<void()> onDoubleClick;

    void mouseDoubleClick(const juce::MouseEvent&) override;

private:
    juce::OpenGLContext glContext;
    GLuint textureId = 0;

    std::unique_ptr<DoomEngine> engine;
    AudioAnalyzer analyzer;
    SignalBus& signalBus;
    const patch::PatchSettingsStore& patchStore;
    SignalRouter router;
    SceneManager sceneManager;

    static constexpr int kWidth = 320;
    static constexpr int kHeight = 200;

    std::vector<float> pullBuffer;

    // Timing
    double lastFrameTime = 0.0;

    // Scene switching
    int lastProgramChange = -1;
    int lastSceneOverride = -1;
    std::atomic<int>* sceneOverridePtr = nullptr;
    std::atomic<int>* currentSceneIndexPtr = nullptr;

    juce::String findWadPath() const;
    juce::String findConfigPath(const juce::String& bundleRoot) const;
    void loadDefaultConfig();
};
