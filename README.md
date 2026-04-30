# DoomViz

A VST3 / AU music visualizer that drives the Doom (1994) software renderer from real-time audio and MIDI. Drop it on a track, your music animates E1M1.

Built on stripped [linuxdoom-1.10](https://github.com/id-Software/DOOM), [JUCE](https://juce.com/), and [yaml-cpp](https://github.com/jbeder/yaml-cpp).

## Screenshots

| Analyzer | Kill Room |
|---|---|
| ![Analyzer](docs/images/analyzer_scene.png) | ![E1M1](docs/images/e1m1_test.png) |

## Scenes

Three modes, switched via the floating control window or MIDI Program Change (0/1/2). Each switch reloads the map for full state isolation.

### Kill Room (PC 0)
Walks E1M1 with the shotgun. Audio onsets spawn monsters in front of the player. Auto-fires at nearby enemies while audio is playing. Sector lighting pulses with RMS. God mode + infinite ammo. Monsters are non-solid so the player walks through them.

### Analyzer (PC 1)
Walks E1M1 with the BFG9000. Eight audio bands are injected as colored bars onto the `STARTAN3` wall texture in real time. Player movement speed tracks the sub-bass band &mdash; bass moves you, silence stops you.

### Acidwarp (PC 2)
2D mode. Pulsing audio-driven warp pattern (layered sin/cos + radial fields) behind eight Doom sprites scaled by frequency band. Each band parameter (zoom, swirl rate, color shift, intensity) is locked to a different band envelope, so the visual moves *with* the music. Sprites: Doom marine (bands 0-1, bass), imp (bands 2-5, mids), zombieman (band 6), shotgun guy (band 7). Onsets jolt the palette.

## How it works

```
Audio In -> FFT / RMS / Onset -> Signal Router -> Scene -> Doom renderer -> OpenGL texture
MIDI In  -> Note / CC / PC    -> Signal Router -> Scene -> Doom renderer -> OpenGL texture
                                       ^                          |
                                  YAML config                  Plugin window
```

Audio passes through unchanged. The plugin is purely visual.

### Audio analysis

- **FFT**: 2048-sample Hann window, 16 log-spaced bands (20Hz&ndash;20kHz)
- **RMS**: time-domain, with per-band envelope follower
- **Onsets**: spectral flux transient detection
- **MIDI**: note velocity, 128 CCs, program change, MIDI clock

Audio thread to render thread is lock-free (`juce::AbstractFifo` ring buffer + atomic MIDI state).

### YAML routing (optional)

Configs in `config/` map analyzer outputs to scene parameters. Without a config, sane audio defaults are auto-wired.

```yaml
inputs:
  kick_env:
    source: audio
    mode: band_rms
    band: [20, 200]
    smoothing: 0.05
    gain: 2.0
routes:
  - from: kick_env
    to: sector_light.all
    scale: 1.0
```

## Building (macOS)

Requires Apple Clang + [Flox](https://flox.dev/).

```bash
git clone --recursive <repo>
cd doom_viz && flox activate
mkdir -p build && cd build
cmake .. -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++
cd ..
flox activate -- cmake --build build -j1 --target DoomViz_VST3
cp -R build/DoomViz_artefacts/VST3/DoomViz.vst3 ~/Library/Audio/Plug-Ins/VST3/
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/VST3/DoomViz.vst3
```

`DOOM1.WAD` (shareware) is auto-bundled into the plugin during build.

### Tests

```bash
flox activate -- bash -c 'cd build && ctest --output-on-failure'
```

The render baseline test verifies the renderer is byte-deterministic against committed PPMs.

## Project layout

```
libs/doom_renderer/   stripped linuxdoom-1.10 as a static C library
src/
  PluginProcessor.*   JUCE AudioProcessor (pass-through + analysis)
  PluginEditor.*      editor window
  DoomViewport.*      OpenGL viewport: texture upload, aspect scaling
  ControlWindow.*     floating scene-switcher in a separate OS window
  audio/              SignalBus, AudioAnalyzer, MidiHandler
  routing/            YAML SignalRouter + RouteConfig
  scenes/             Scene base + KillRoom, AnalyzerRoom, Spectrum2
  doom/               C++ wrapper around the C renderer
config/               default YAML configs
test/                 render harness + baseline PPMs + test audio
extern/               JUCE and yaml-cpp submodules
```

## Doom renderer extensions

The C renderer was extended with:

- `doom_move_player` &mdash; collision-checked movement via `P_TryMove`
- `doom_set_camera_angle` &mdash; rotate without relinking the blockmap
- `doom_set_wall_texture_data` &mdash; replace a wall texture's column data at runtime
- `doom_set_flat_data` &mdash; replace a floor / ceiling flat
- `doom_get_sprite` &mdash; pull raw `patch_t` data for sprite drawing
- `doom_fire_weapon` / `doom_give_weapon` &mdash; weapon control
- `doom_set_god_mode` / `doom_respawn_player` &mdash; player state

## License

Doom source is GPL. `DOOM1.WAD` (shareware) is freely redistributable per id Software's terms. This project is open source and noncommercial.
