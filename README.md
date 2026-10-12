# HardTune

A real-time pitch-correction audio plugin written in C++. Sing into it and it snaps your voice to the nearest
note in a chosen key: at a fast retune speed you get the hard, robotic vocal sound of modern trap and rap, and at
slower speeds it gently corrects pitch. It builds as a VST3 plugin (and AU on macOS) for DAWs like FL Studio,
Ableton Live and Logic Pro, plus a standalone app.

> **Status:** early development. The plugin builds and passes audio through; pitch detection and shifting are
> in progress (see the roadmap).

## How it works

1. **Pitch detection:** estimate the sung pitch with the YIN algorithm.
2. **Target note:** snap that pitch to the nearest note of the selected key and scale
   (`hardtune::nearestScaleNote` in [`dsp/`](dsp/)).
3. **Pitch shifting:** shift the voice to the target with PSOLA (pitch-synchronous overlap-add), which keeps the
   voice sounding natural.
4. **Retune speed:** glide toward the target over 0-200 ms; 0 ms gives the instant, robotic jump.

All processing runs on the real-time audio thread, so the DSP code never allocates memory or takes locks while
processing audio.

## Project layout

| Folder | Contents |
|---|---|
| `dsp/` | Pure C++20 DSP library with no JUCE dependency |
| `plugin/` | JUCE plugin wrapper: parameters, audio callback, editor |
| `tests/` | GoogleTest unit tests for the DSP library |

## Building

Requires CMake 3.22+ and a C++20 compiler (Visual Studio 2022 on Windows, Xcode on macOS, GCC or Clang on Linux).
JUCE and GoogleTest are downloaded automatically during configuration.

```bash
# Fast: DSP library and unit tests only
cmake --preset dsp-only
cmake --build --preset dsp-only
ctest --preset dsp-only

# Full: plugin and tests
cmake --preset default
cmake --build --preset default
```

The built plugin ends up in `build/default/plugin/HardTune_artefacts/Release/`.

## Roadmap

- [x] Project setup: CMake, JUCE plugin shell, GoogleTest, CI on Windows, macOS and Linux
- [x] Frequency/MIDI conversion and scale snapping, with tests
- [ ] YIN pitch detection, tested on synthetic tones and recorded vocals
- [ ] PSOLA pitch shifting
- [ ] Retune speed and formant preservation
- [ ] Latency and CPU benchmarks
- [ ] Custom UI and demo video

## License

HardTune is licensed under the [GNU Affero General Public License v3.0](LICENSE), because it is built with
[JUCE](https://juce.com), whose open-source license is AGPLv3. You can use, study and modify the code freely;
if you distribute a modified version, you must share its source under the same license.
