#include "DoomViewport.h"

// Subset of the Doom PLAYPAL palette for the test pattern
static const uint8_t kDoomPalette[][3] = {
    {0,   0,   0},    // 0: black
    {31,  23,  11},   // 1: dark brown
    {23,  15,  7},    // 2: darker brown
    {75,  75,  75},   // 3: grey
    {255, 255, 255},  // 4: white
    {27,  27,  27},   // 5: dark grey
    {19,  19,  19},   // 6: darker grey
    {11,  11,  11},   // 7: near black
    {7,   7,   7},    // 8: near black
    {47,  55,  31},   // 9: dark green
    {35,  43,  15},   // 10: darker green
    {23,  31,  7},    // 11: olive
    {15,  23,  0},    // 12: dark olive
    {79,  59,  43},   // 13: tan
    {71,  51,  35},   // 14: brown
    {63,  43,  27},   // 15: brown
    {255, 183, 115},  // 16: peach
    {235, 159, 95},   // 17: orange
    {215, 135, 75},   // 18: dark orange
    {195, 115, 59},   // 19: rust
    {175, 95,  43},   // 20: dark rust
    {155, 79,  31},   // 21: darker rust
    {135, 63,  19},   // 22: brown
    {187, 115, 159},  // 23: pink
    {175, 99,  143},  // 24: dark pink
    {163, 83,  127},  // 25: mauve
    {151, 67,  111},  // 26: dark mauve
    {139, 55,  99},   // 27: purple-red
    {183, 0,   0},    // 28: red
    {163, 0,   0},    // 29: dark red
    {143, 0,   0},    // 30: darker red
    {123, 0,   0},    // 31: deep red
};

DoomViewport::DoomViewport()
{
    glContext.setRenderer(this);
    glContext.setContinuousRepainting(true);
    glContext.attachTo(*this);
}

DoomViewport::~DoomViewport()
{
    glContext.detach();
}

void DoomViewport::generateTestPattern()
{
    testPattern.resize(kWidth * kHeight * 4);

    for (int y = 0; y < kHeight; ++y)
    {
        for (int x = 0; x < kWidth; ++x)
        {
            int idx = (y * kWidth + x) * 4;

            // Vertical color bars using Doom palette colors
            int paletteIdx = (x * 32) / kWidth;

            // Modulate brightness by vertical position (darker at top/bottom)
            float brightness = 1.0f - 0.5f * std::abs((y - kHeight / 2.0f) / (kHeight / 2.0f));

            testPattern[idx + 0] = (uint8_t)(kDoomPalette[paletteIdx][0] * brightness);
            testPattern[idx + 1] = (uint8_t)(kDoomPalette[paletteIdx][1] * brightness);
            testPattern[idx + 2] = (uint8_t)(kDoomPalette[paletteIdx][2] * brightness);
            testPattern[idx + 3] = 255;
        }
    }
}

void DoomViewport::newOpenGLContextCreated()
{
    generateTestPattern();

    juce::gl::glGenTextures(1, &textureId);
    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MIN_FILTER, juce::gl::GL_NEAREST);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MAG_FILTER, juce::gl::GL_NEAREST);
    juce::gl::glTexImage2D(juce::gl::GL_TEXTURE_2D, 0, juce::gl::GL_RGBA,
                           kWidth, kHeight, 0,
                           juce::gl::GL_RGBA, juce::gl::GL_UNSIGNED_BYTE,
                           testPattern.data());
}

void DoomViewport::renderOpenGL()
{
    juce::gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

    juce::gl::glEnable(juce::gl::GL_TEXTURE_2D);
    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);

    // Calculate aspect-correct scaling (maintain 320:200 = 8:5 aspect)
    auto bounds = getLocalBounds().toFloat();
    float doomAspect = 320.0f / 200.0f;
    float viewAspect = bounds.getWidth() / bounds.getHeight();

    float scaleX, scaleY;
    if (viewAspect > doomAspect)
    {
        // Window is wider than Doom — pillarbox
        scaleY = 1.0f;
        scaleX = doomAspect / viewAspect;
    }
    else
    {
        // Window is taller than Doom — letterbox
        scaleX = 1.0f;
        scaleY = viewAspect / doomAspect;
    }

    // Draw fullscreen quad with texture
    juce::gl::glBegin(juce::gl::GL_QUADS);
    juce::gl::glTexCoord2f(0.0f, 1.0f); juce::gl::glVertex2f(-scaleX, -scaleY);
    juce::gl::glTexCoord2f(1.0f, 1.0f); juce::gl::glVertex2f(scaleX, -scaleY);
    juce::gl::glTexCoord2f(1.0f, 0.0f); juce::gl::glVertex2f(scaleX, scaleY);
    juce::gl::glTexCoord2f(0.0f, 0.0f); juce::gl::glVertex2f(-scaleX, scaleY);
    juce::gl::glEnd();
}

void DoomViewport::openGLContextClosing()
{
    if (textureId != 0)
    {
        juce::gl::glDeleteTextures(1, &textureId);
        textureId = 0;
    }
    testPattern.clear();
}
