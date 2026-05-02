#include "DoomViewport.h"
#include "scenes/KillRoomScene.h"
#include "scenes/AnalyzerRoomScene.h"
#include "scenes/Spectrum2Scene.h"

DoomViewport::DoomViewport(SignalBus& bus, double sampleRate,
                           const patch::VisualizerState& state,
                           std::atomic<int>* sceneOverride,
                           std::atomic<int>* currentSceneIndex)
    : signalBus(bus), vizState(state),
      pullBuffer(8192, 0.0f),
      sceneOverridePtr(sceneOverride),
      currentSceneIndexPtr(currentSceneIndex)
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

void DoomViewport::mouseDoubleClick(const juce::MouseEvent&)
{
    if (onDoubleClick)
        onDoubleClick();
}

void DoomViewport::setSampleRate(double sr)
{
    analyzer.setSampleRate(sr);
}

juce::String DoomViewport::findWadPath() const
{
    auto thisModule = juce::File::getSpecialLocation(
        juce::File::currentExecutableFile);
    auto bundleRoot = thisModule.getParentDirectory()
                                .getParentDirectory()
                                .getParentDirectory();

    auto bundleResources = bundleRoot.getChildFile("Contents/Resources/DOOM1.WAD");
    if (bundleResources.existsAsFile())
        return bundleResources.getFullPathName();

    auto nextToBundle = bundleRoot.getParentDirectory().getChildFile("DOOM1.WAD");
    if (nextToBundle.existsAsFile())
        return nextToBundle.getFullPathName();

    auto devPath = juce::File::getCurrentWorkingDirectory()
                       .getChildFile("resources/DOOM1.WAD");
    if (devPath.existsAsFile())
        return devPath.getFullPathName();

    return {};
}

juce::String DoomViewport::findConfigPath(const juce::String& bundleRoot) const
{
    // Check bundle Resources
    auto bundleCfg = juce::File(bundleRoot).getChildFile("Contents/Resources/default_killroom.yaml");
    if (bundleCfg.existsAsFile())
        return bundleCfg.getFullPathName();

    // Check config/ relative to CWD
    auto devCfg = juce::File::getCurrentWorkingDirectory()
                      .getChildFile("config/default_killroom.yaml");
    if (devCfg.existsAsFile())
        return devCfg.getFullPathName();

    return {};
}

void DoomViewport::loadDefaultConfig()
{
    auto thisModule = juce::File::getSpecialLocation(
        juce::File::currentExecutableFile);
    auto bundleRoot = thisModule.getParentDirectory()
                                .getParentDirectory()
                                .getParentDirectory();

    auto configPath = findConfigPath(bundleRoot.getFullPathName());
    if (configPath.isNotEmpty())
    {
        RouteConfig cfg;
        if (cfg.loadFromFile(configPath.toStdString()))
        {
            router.loadConfig(cfg.getConfig());
            DBG("Loaded config: " + configPath);
        }
    }
    else
    {
        DBG("No config file found, using defaults");
    }
}

void DoomViewport::newOpenGLContextCreated()
{
    // Only init the doom engine + scenes once. Context recreation (e.g. when
    // the viewport is reparented for fullscreen) must not reset visualizer state.
    if (! engine)
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
        }

        sceneManager.addScene(std::make_unique<KillRoomScene>(analyzer, vizState));
        sceneManager.addScene(std::make_unique<AnalyzerRoomScene>(analyzer, signalBus));
        sceneManager.addScene(std::make_unique<Spectrum2Scene>(analyzer, vizState));

        if (engine->isMapLoaded())
            sceneManager.init(*engine);

        loadDefaultConfig();

        lastFrameTime = juce::Time::getMillisecondCounterHiRes() / 1000.0;
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
    // Timing
    double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    float deltaTime = static_cast<float>(now - lastFrameTime);
    lastFrameTime = now;
    deltaTime = std::min(deltaTime, 0.1f); // cap to avoid huge jumps

    // Pull audio from the signal bus and feed to analyzer
    int pulled = signalBus.pullAudioSamples(pullBuffer.data(),
                                             static_cast<int>(pullBuffer.size()));
    if (pulled > 0)
        analyzer.pushSamples(pullBuffer.data(), pulled);

    // Evaluate routes (if config loaded) or build default parameter map
    router.evaluate(analyzer, signalBus, deltaTime);

    // Build the parameter map: use router output, then overlay direct audio values
    auto params = router.getParameters();

    // Always inject direct audio analysis so scenes react even without YAML config
    float rms = analyzer.getRMSLevel();
    bool onset = analyzer.getOnset();
    const float* bands = analyzer.getBandRMS();
    const float* envelope = analyzer.getBandEnvelope();

    // Use RMS directly (0.0-1.0 range for typical audio) — don't over-amplify
    // so that dynamics are preserved (mastered music RMS is ~0.2-0.5)
    if (params.find("sector_light.all") == params.end())
        params["sector_light.all"] = std::min(1.0f, rms * 2.0f);
    if (params.find("monster_spawn") == params.end())
        params["monster_spawn"] = onset ? 1.0f : 0.0f;
    if (params.find("camera_shake") == params.end())
        params["camera_shake"] = std::min(1.0f, rms);
    if (params.find("player_speed") == params.end())
        params["player_speed"] = std::min(1.0f, rms * 2.0f);

    // Per-band amplitudes for spectrum scene
    for (int i = 0; i < analyzer.getNumBands() && i < 16; ++i)
    {
        std::string key = "band." + std::to_string(i) + ".amplitude";
        if (params.find(key) == params.end())
            params[key] = envelope[i];
    }

    // MIDI velocity → light boost
    float vel = static_cast<float>(signalBus.getLastVelocity()) / 127.0f;
    if (vel > params["sector_light.all"])
        params["sector_light.all"] = vel;

    // Scene switching: control window override takes priority, then MIDI PC.
    // After any successful switch, mirror the new index into the shared atomic
    // so the editor's polling Timer can update the patch window even when the
    // switch was driven by MIDI rather than the GUI.
    auto publishCurrent = [this](int idx) {
        if (currentSceneIndexPtr)
            currentSceneIndexPtr->store(idx, std::memory_order_relaxed);
    };

    if (sceneOverridePtr)
    {
        int override = sceneOverridePtr->load(std::memory_order_relaxed);
        if (override >= 0 && override != lastSceneOverride && engine && engine->isMapLoaded())
        {
            lastSceneOverride = override;
            int idx = override % sceneManager.getNumScenes();
            sceneManager.switchTo(idx, *engine);
            publishCurrent(idx);
        }
    }

    int pc = signalBus.getProgramChange();
    if (pc != lastProgramChange && engine && engine->isMapLoaded())
    {
        lastProgramChange = pc;
        int idx = pc % sceneManager.getNumScenes();
        sceneManager.switchTo(idx, *engine);
        publishCurrent(idx);
    }

    juce::gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    juce::gl::glClear(juce::gl::GL_COLOR_BUFFER_BIT);

    const uint8_t* rgba = nullptr;

    if (engine && engine->isMapLoaded() && sceneManager.getNumScenes() > 0)
    {
        rgba = sceneManager.updateAndRender(*engine, params, deltaTime);
    }

    if (rgba)
    {
        juce::gl::glBindTexture(juce::gl::GL_TEXTURE_2D, textureId);
        juce::gl::glTexSubImage2D(juce::gl::GL_TEXTURE_2D, 0, 0, 0,
                                  kWidth, kHeight,
                                  juce::gl::GL_RGBA, juce::gl::GL_UNSIGNED_BYTE,
                                  rgba);
    }

    // Draw the texture
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
    // Don't reset engine — it must survive context recreation for fullscreen
    // toggling. Engine cleanup happens in the destructor.
}
