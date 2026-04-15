#include "DoomViewport.h"

DoomViewport::DoomViewport(SignalBus& bus, double sampleRate)
    : signalBus(bus), pullBuffer(8192, 0.0f)
{
    analyzer.setSampleRate(sampleRate);
    glContext.setRenderer(this);
    glContext.setContinuousRepainting(true);
    glContext.attachTo(*this);
}

DoomViewport::~DoomViewport()
{
    glContext.detach();
}

void DoomViewport::setSampleRate(double sr)
{
    analyzer.setSampleRate(sr);
}

juce::String DoomViewport::findWadPath() const
{
    // For VST3/AU: find our own bundle via the binary's location
    auto thisModule = juce::File::getSpecialLocation(
        juce::File::currentExecutableFile);
    auto bundleRoot = thisModule.getParentDirectory()
                                .getParentDirectory()
                                .getParentDirectory();

    // Check Resources inside our own bundle
    auto bundleResources = bundleRoot.getChildFile("Contents/Resources/DOOM1.WAD");
    if (bundleResources.existsAsFile())
        return bundleResources.getFullPathName();

    // Check next to the plugin bundle
    auto nextToBundle = bundleRoot.getParentDirectory().getChildFile("DOOM1.WAD");
    if (nextToBundle.existsAsFile())
        return nextToBundle.getFullPathName();

    // Check resources/ relative to CWD (dev builds)
    auto devPath = juce::File::getCurrentWorkingDirectory()
                       .getChildFile("resources/DOOM1.WAD");
    if (devPath.existsAsFile())
        return devPath.getFullPathName();

    return {};
}

void DoomViewport::newOpenGLContextCreated()
{
    engine = std::make_unique<DoomEngine>();

    auto wadPath = findWadPath();
    if (wadPath.isNotEmpty() && engine->init(wadPath.toStdString()))
    {
        engine->loadMap(1, 1);
    }
    else
    {
        DBG("DoomViewport: Could not find or load DOOM1.WAD");
        DBG("  Searched: " + wadPath);
    }

    juce::gl::glGenTextures(1, &textureId);
    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MIN_FILTER, juce::gl::GL_NEAREST);
    juce::gl::glTexParameteri(juce::gl::GL_TEXTURE_2D, juce::gl::GL_TEXTURE_MAG_FILTER, juce::gl::GL_NEAREST);
    juce::gl::glTexImage2D(juce::gl::GL_TEXTURE_2D, 0, juce::gl::GL_RGBA,
                           kWidth, kHeight, 0,
                           juce::gl::GL_RGBA, juce::gl::GL_UNSIGNED_BYTE,
                           nullptr);
}

void DoomViewport::renderOpenGL()
{
    // Pull audio from the signal bus and feed to analyzer
    int pulled = signalBus.pullAudioSamples(pullBuffer.data(),
                                             static_cast<int>(pullBuffer.size()));
    if (pulled > 0)
        analyzer.pushSamples(pullBuffer.data(), pulled);

    juce::gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

    if (engine && engine->isMapLoaded())
    {
        engine->tick();

        const uint8_t* rgba = engine->renderFrame();

        juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);
        juce::gl::glTexSubImage2D(juce::gl::GL_TEXTURE_2D, 0, 0, 0,
                                  kWidth, kHeight,
                                  juce::gl::GL_RGBA, juce::gl::GL_UNSIGNED_BYTE,
                                  rgba);
    }

    juce::gl::glEnable(juce::gl::GL_TEXTURE_2D);
    juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);

    auto bounds = getLocalBounds().toFloat();
    float doomAspect = 320.0f / 200.0f;
    float viewAspect = bounds.getWidth() / bounds.getHeight();

    float scaleX, scaleY;
    if (viewAspect > doomAspect)
    {
        scaleY = 1.0f;
        scaleX = doomAspect / viewAspect;
    }
    else
    {
        scaleX = 1.0f;
        scaleY = viewAspect / doomAspect;
    }

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

    engine.reset();
}
