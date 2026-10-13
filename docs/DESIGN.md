# HardTune design

HardTune is a real-time pitch-correction plugin: it detects the pitch of a sung voice, picks the nearest note in a
chosen key, and shifts the voice to that note while it plays. This document records the main design decisions, the
alternatives considered, and why each choice was made.

## Goals

- **Real time:** process live vocals with at most 10 ms of algorithmic latency (Alto/Tenor voice type, 48 kHz) and
  under 2% of one CPU core.
- **Sound:** a clean "hard-tune" effect at 0 ms retune speed and natural correction at slower speeds, without
  warbling, clicks or the "chipmunk" effect.
- **Correct by construction:** no allocations, locks or I/O on the audio thread, enforced by tooling in CI.
- **Measured:** every performance and accuracy claim comes from a script anyone can rerun.

**Non-goals:** polyphonic input (chords, full mixes), pitch shifts larger than about 5 semitones, and neural-network
models that can't run in real time on a CPU.

## Architecture

```
               host audio thread (hard deadline: 2.67 ms per 128 samples at 48 kHz)
 input ──► ring buffer ──► every 128 samples (fixed internal hop):
                             1. downsample to 16 kHz
                             2. YIN pitch detection ──► voiced? confident?
                             3. target note (scale snap + hysteresis) ──► retune smoothing
                             4. TD-PSOLA pitch shift (pitch marks from step 2)
                         ──► mix (smoothed) ──► output
                                   │
                                   └─► lock-free SPSC queue ──► UI thread: pitch graph
 UI thread ──► parameters (std::atomic, via JUCE) ──► audio thread
```

| Layer | Folder | Depends on | Notes |
|---|---|---|---|
| DSP core | `dsp/` | C++20 standard library, PFFFT | No JUCE; real-time safe; unit tested |
| Plugin | `plugin/` | JUCE, DSP core | Parameters, audio callback, editor |
| Python bindings | `python/` | pybind11, DSP core | Used by the evaluation scripts |
| Browser demo | `web/` | Emscripten, DSP core | Runs in an AudioWorklet |

Keeping the DSP core free of JUCE lets the same code run in the plugin, in Python and in the browser, and keeps the
unit tests fast.

## Decisions

### 1. Pitch detection: YIN

| Option | Why not |
|---|---|
| Autocorrelation | Frequent octave errors |
| MPM (McLeod & Wyvill, 2005) | Very similar to YIN; little to gain |
| pYIN (Mauch & Dixon, 2014) | Needs lookahead for its HMM, which adds latency |
| CREPE (neural, 2018) | Too slow for real time on a CPU |
| **YIN (de Cheveigné & Kawahara, 2002)** | **Chosen:** accurate on vocals, cheap, well documented |

Implemented twice: a direct O(N·W) version as the reference, then an FFT-based O(N log N) version that must produce
the same output (see 6b). The search range comes from the Voice Type setting (decision 3). Threshold starts at 0.10
to 0.15 as in the paper and is tuned with tests; parabolic interpolation refines the period to sub-sample precision.

### 2. Pitch shifting: TD-PSOLA

| Option | Why not |
|---|---|
| Resampling | Changes duration and formants ("chipmunk" voice) |
| Phase vocoder | Phasiness and transient smearing, high latency, needs separate formant correction |
| WSOLA | Designed for time stretching rather than pitch shifting |
| Neural vocoder | Too slow for real time |
| **TD-PSOLA (Moulines & Charpentier, 1990)** | **Chosen:** natural on monophonic voice, preserves formants, about one period of latency, cheap |

Each grain is two pitch periods long and keeps its original content; only the grain spacing changes, which is why
formants are preserved. Pitch marks start at a fixed period spacing from the YIN estimate; a second version aligns
marks to the waveform's peak within each period, and the two are compared with listening tests and a roughness
measure. Quality degrades beyond about ±5 semitones, which is outside the use case (corrections are usually under
one semitone).

### 3. Latency: a Voice Type setting

Algorithmic latency is set by the lowest pitch to support, because PSOLA needs one full period of lookahead:

| Voice Type | Lowest pitch | Latency at 48 kHz |
|---|---|---|
| Soprano | 150 Hz | ~7 ms (320 samples) |
| Alto/Tenor | 100 Hz | ~10 ms (480 samples) |
| Low Male | 70 Hz | ~14 ms (686 samples) |

A single fixed range (e.g. 60 Hz upward) would cost every singer 17 ms. The Voice Type setting also narrows YIN's
search range, which lowers CPU use and octave errors. The plugin reports its exact latency with
`setLatencySamples()` so the DAW compensates during mixing, and an automated test checks that the measured latency
matches the reported value.

### 4. Real-time threading and memory

Rule: nothing on the audio thread may block, allocate, lock, log or throw.

- **Memory:** all buffers are allocated in `prepareToPlay`, sized for the maximum block size and the lowest Voice
  Type. Ring buffers have power-of-two sizes so index wrapping is a bit mask.
- **Audio to UI:** a single-producer/single-consumer lock-free queue (our own template using `std::atomic` with
  acquire/release ordering) carries pitch data to the UI. A mutex could block the audio thread, and JUCE's
  `AbstractFifo` would work but hides the design.
- **UI to audio:** parameters are `std::atomic<float>` values through JUCE's `AudioProcessorValueTreeState`.
- **Any block size:** input goes into a ring buffer and analysis runs on a fixed 128-sample hop, so output does not
  depend on the host's block size. A test processes the same audio in random block sizes and requires bit-identical
  output.
- **Enforcement:** a CI job builds with Clang's RealtimeSanitizer, which fails on any allocation or lock inside
  functions marked real-time.

### 5. Retune behavior

- **Voiced/unvoiced:** correct only when YIN is confident and the signal is loud enough; otherwise pass audio
  through. Switching crossfades over a few milliseconds to avoid clicks.
- **Target note hysteresis:** switch target notes only after the pitch passes the midpoint by about 20 cents, so a
  pitch between two notes doesn't warble.
- **Retune speed:** a one-pole smoother on the target pitch in cents (log frequency), with time constant set by the
  Retune Speed parameter; 0 ms jumps instantly for the hard-tune sound.
- **Outlier rejection:** a jump larger than about 7 semitones is ignored unless YIN is highly confident. Chosen over a
  3-frame median filter, which would add about 2.7 ms of latency.
- **Mix smoothing:** the Mix parameter ramps over about 20 ms to avoid zipper noise.

### 6. Performance

Measure first: Google Benchmark is set up before any optimization, and every optimization is checked against it.

- **6a. Downsample before pitch detection.** Vocal pitch stays below about 1 kHz, so YIN runs on audio low-pass
  filtered and decimated to 16 kHz: 3× fewer samples over 3× fewer lags, about 9× less work for the direct YIN.
  Parabolic interpolation keeps precision; a test bounds the accuracy loss in cents.
- **6b. FFT-based YIN** using **PFFFT** (BSD license, SIMD-optimized, small) behind our own FFT interface, so the
  library can be swapped. FFTW was rejected for its build complexity across three platforms; a hand-written FFT for
  speed.
- **6c. SIMD.** Hot loops are written for compiler auto-vectorization (contiguous float arrays, simple loops, no
  aliasing) and checked with the compiler's vectorization reports. Hand-written intrinsics (AVX and NEON) only where
  profiling shows a need.
- **6d. Precision.** 32-bit floats for audio; 64-bit accumulators only where rounding error builds up (YIN's
  cumulative sums), validated against a 64-bit reference.

### 7. Testing and evaluation

- **Unit tests (GoogleTest, every push):** pure and harmonic-rich tones, tones with a missing fundamental, silence and
  noise, a pitch exactly between two notes, vibrato and glides, and a +1 semitone shift (output must measure +100
  cents with unchanged duration).
- **Pitch accuracy on real singing:** MIR-1K (1,000 sung clips with pitch labels), reporting raw pitch accuracy
  (frames within 50 cents) and voicing accuracy, compared against librosa's pYIN. The dataset is downloaded by a
  script and not committed.
- **Shifting quality:** shifted vocals measured with an independent pitch tracker (within ±10 cents of the target), a
  spectral-envelope distance to check formant preservation, and before/after audio clips.
- **Real-time safety:** RealtimeSanitizer, the random-block-size test, the latency test, a ThreadSanitizer stress
  test of the lock-free queue, and AddressSanitizer/UndefinedBehaviorSanitizer on all tests.
- **Plugin validation:** pluginval at high strictness in CI.

### 8. Benchmarks and published results

| Metric | Target | Measured by |
|---|---|---|
| Latency (Alto/Tenor) | ≤ 10 ms | Automated latency test |
| CPU | < 2% of one core | Google Benchmark vs. the 2.67 ms block deadline |
| Pitch detection speedup | Report measured | Direct vs. downsampled vs. FFT YIN, identical output |
| Pitch accuracy | Report measured, vs. librosa pYIN | MIR-1K raw pitch accuracy |
| Correction accuracy | ±10 cents | Independent tracker on shifted output |
| Reliability | 0 failures | pluginval and all sanitizers |

Benchmarks run in CI and fail the build when CPU time regresses by more than about 10% against a baseline from the
same runner type. Results, charts and a demo video go in the README; tagged versions publish Windows and macOS builds
to GitHub Releases.

## Add-ons

Built after the core plugin works, in this order:

1. **Python bindings (pybind11):** `import hardtune` exposes pitch detection and shifting. The MIR-1K evaluation
   calls the C++ code through these bindings, and the package can be installed with pip.
2. **Browser demo (Emscripten + Web Audio AudioWorklet):** the DSP core compiled to WebAssembly, so anyone can try
   the effect on their own voice from a link, with nothing to install.
3. **Live pitch graph:** the plugin UI plots sung and corrected pitch over time, fed by the lock-free queue.
4. **Technical write-up:** what was measured and learned, based on this document.

## Milestones

1. Project setup: CMake, JUCE plugin shell, GoogleTest, CI *(done)*
2. Scale snapping and frequency conversion *(done)*
3. Benchmark harness, ring buffer, lock-free queue, fixed-hop processing, RealtimeSanitizer job
4. YIN (direct), unit tests, Voice Type setting
5. Downsampling and FFT-based YIN, with measured speedups
6. TD-PSOLA (fixed spacing, then peak-aligned), latency reporting and test
7. Retune behavior (voicing, hysteresis, smoothing, outlier rejection), pluginval in CI
8. Python bindings and MIR-1K evaluation
9. Browser demo
10. Pitch graph UI, demo video, README results, first release
11. Technical write-up
