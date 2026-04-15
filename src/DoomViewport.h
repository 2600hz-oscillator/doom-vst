#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include "doom/DoomEngine.h"
#include "audio/AudioAnalyzer.h"
#include "audio/SignalBus.h"
#include <memory>

class DoomViewport : public juce::Component,
                     public juce::OpenGLRenderer
{
public:
    DoomViewport(SignalBus& signalBus, double sampleRate);
    ~DoomViewport() override;

    void setSampleRate(double sr);

    // OpenGLRenderer callbacks
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // Access analyzer results (for scenes to read)
    const AudioAnalyzer& getAnalyzer() const { return analyzer; }

private:
    juce::OpenGLContext glContext;
    GLuint textureId = 0;

    std::unique_ptr<DoomEngine> engine;
    AudioAnalyzer analyzer;
    SignalBus& signalBus;

    static constexpr int kWidth = 320;
    static constexpr int kHeight = 200;

    // Temp buffer for pulling audio from ring buffer
    std::vector<float> pullBuffer;

    juce::String findWadPath() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoomViewport)
};
