#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include "routing/SignalRouter.h"
#include "routing/RouteConfig.h"
#include "scenes/SceneManager.h"
#include <memory>

class DoomViewport : public juce::Component,
                     public juce::OpenGLRenderer
{
public:
    DoomViewport(SignalBus& signalBus, double sampleRate,
                 std::atomic<int>* sceneOverride = nullptr);
    ~DoomViewport() override;

    void setSampleRate(double sr);

    // OpenGLRenderer callbacks
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // Access analyzer results
    const AudioAnalyzer& getAnalyzer() const { return analyzer; }

private:
    juce::OpenGLContext glContext;
    GLuint textureId = 0;

    std::unique_ptr<DoomEngine> engine;
    AudioAnalyzer analyzer;
    SignalBus& signalBus;
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

    juce::String findWadPath() const;
    juce::String findConfigPath(const juce::String& bundleRoot) const;
    void loadDefaultConfig();
};
