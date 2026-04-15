# DoomViz

A VST3/AU music visualizer plugin that embeds the Doom (1994) software renderer and drives it from real-time audio and MIDI input. Load it as an effect in your DAW, route audio through it, and watch E1M1 come alive with your music.

Built on the stripped [linuxdoom-1.10](https://github.com/id-Software/DOOM) renderer, [JUCE](https://juce.com/) for the plugin framework, and [yaml-cpp](https://github.com/jbeder/yaml-cpp) for configuration.

## How It Works

Audio flows through the plugin unchanged while a parallel analysis pipeline extracts:
- **FFT spectrum** (2048-sample, 16 log-spaced frequency bands)
- **RMS level** (time-domain, per-frame)
- **Onset detection** (spectral flux transient trigger)
- **MIDI state** (note velocity, CC values, program change, clock)

A YAML routing engine maps these signals to visual parameters. Scenes read the parameter map each frame to drive the Doom renderer.

```
Audio In -> FFT/RMS/Onset -> Signal Router -> Scene Controller -> Doom Renderer -> OpenGL Texture
MIDI In  -> Note/CC/PC    ->      ^                                                     |
                              YAML Config                                          Plugin Window
```

## Visualizer Modes

### Scene A: Kill Room (default)
The player auto-navigates E1M1 with collision detection. Audio onsets spawn monsters ahead of the player. The player automatically faces and shoots nearby monsters when audio is playing. Sector lighting pulses with RMS level. Camera speed increases with audio energy.

### Scene B: Sprite Spectrum
A 2D mode that draws Doom sprites as frequency band bars. Each of 8 bands is represented by a colored block using the Doom palette, scaled vertically by the band's amplitude.

### Scene C: Analyzer Room
The player auto-navigates E1M1 while the FFT spectrum is rendered onto the NUKAGE1 floor texture as a real-time bar graph with a green-to-red gradient.

Switch scenes by sending MIDI Program Change 0/1/2 to the plugin.

## E1M1 Test Renders

These are generated deterministically by the render test suite:

| E1M1 Starting View | E1M1 With Spawned Imps |
|---|---|
| ![E1M1](docs/images/e1m1_test.png) | ![E1M1 with imps](docs/images/e1m1_with_imp.png) |

## Building

### Prerequisites

- macOS with Apple Clang (Xcode Command Line Tools)
- [Flox](https://flox.dev/) for dependency management
- DOOM1.WAD (shareware) in `resources/`

### Build Steps

```bash
# Clone with submodules
git clone --recursive <repo-url>
cd doom_viz

# Activate the flox environment (provides cmake, git-lfs, python3)
flox activate

# Configure (must use Apple Clang for JUCE Objective-C++ support)
mkdir -p build && cd build
cmake .. -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++
cd ..

# Build standalone app
flox activate -- cmake --build build -j1 --target DoomViz_Standalone

# Build VST3 plugin
# (may need to pre-generate PkgInfo if juceaide gets killed by macOS sandbox)
cd build && ./extern/JUCE/tools/extras/Build/juceaide/juceaide_artefacts/Custom/juceaide pkginfo VST3 DoomViz_artefacts/JuceLibraryCode/DoomViz_VST3/PkgInfo && cd ..
flox activate -- cmake --build build -j1 --target DoomViz_VST3

# Install VST3
cp -R build/DoomViz_artefacts/VST3/DoomViz.vst3 ~/Library/Audio/Plug-Ins/VST3/
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/DoomViz.vst3
```

### Run Tests

```bash
flox activate -- bash -c 'cd build && ctest --output-on-failure'
```

## Usage in a DAW

1. Build and install the VST3 (see above)
2. In your DAW, add DoomViz as an effect on an audio track
3. Route audio through the track — the plugin passes audio through unchanged
4. The plugin window shows the Doom renderer reacting to your audio
5. Send MIDI Program Change to switch scenes (0 = Kill Room, 1 = Sprite Spectrum, 2 = Analyzer Room)

### YAML Configuration

Scene routing is configured via YAML files in `config/`. The default config maps:
- Low frequency RMS (20-200Hz) to sector lighting
- Audio onsets to monster spawning
- Overall RMS to camera shake
- MIDI velocity to lighting boost
- MIDI CC1 to palette flash

See `config/default_killroom.yaml` for the full schema.

## Architecture

```
doom_viz/
  libs/doom_renderer/     # Stripped linuxdoom-1.10 as a static C library
  src/
    PluginProcessor.*     # JUCE AudioProcessor (audio pass-through + analysis)
    PluginEditor.*        # JUCE editor window
    DoomViewport.*        # OpenGL renderer (texture upload + aspect scaling)
    audio/                # SignalBus, AudioAnalyzer, MidiHandler
    routing/              # YAML-driven SignalRouter + RouteConfig
    scenes/               # Scene interface + KillRoom, SpriteSpectrum, AnalyzerRoom
    doom/                 # DoomEngine C++ wrapper
  config/                 # YAML scene configs
  test/                   # Render test harness + baselines + test audio
  extern/                 # JUCE and yaml-cpp submodules
```

## License

The Doom source code is released under the [GPL](https://github.com/id-Software/DOOM/blob/master/linuxdoom-1.10/DOOMLIC.TXT). DOOM1.WAD (shareware) is freely redistributable per id Software's original terms. This project is open source and noncommercial.
