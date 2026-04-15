#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

class DoomViewport : public juce::Component,
                     public juce::OpenGLRenderer
{
public:
    DoomViewport();
    ~DoomViewport() override;

    // OpenGLRenderer callbacks
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

private:
    juce::OpenGLContext glContext;
    GLuint textureId = 0;

    // Test pattern: 320x200 RGBA
    static constexpr int kWidth = 320;
    static constexpr int kHeight = 200;
    std::vector<uint8_t> testPattern;

    void generateTestPattern();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoomViewport)
};
