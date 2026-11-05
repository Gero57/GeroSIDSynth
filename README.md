# Gero SID Synth

A native Apple Silicon VST3 instrument for Cubase, built around faithful MOS 6581 / MOS 8580 SID emulation.

## Project status

**Stage 1 — VST3 build scaffold committed.** The repository now contains a JUCE/CMake plugin target and a GitHub Actions workflow that cross-compiles an arm64 VST3 on macOS. This first scaffold is intentionally silent: it validates the plugin/build pipeline and is not yet an audible SID instrument.

## Target

- macOS on Apple Silicon (arm64), initially tested on an M-series Mac.
- VST3 instrument for Cubase.
- JUCE 8 + CMake.
- One SID chip instance with its three original independent voices.
- PAL clock: 985,248 Hz (985.248 kHz).
- Selectable 6581 and 8580 models.
- MIDI note control for the three SID voices.

## Planned sound features

- Three SID voices, each with its own frequency, pulse width, waveform and gate.
- Triangle, sawtooth, pulse and noise waveforms.
- SID ADSR envelopes (per voice).
- Hard sync and ring modulation.
- Authentic SID filter model and chip-specific characteristics.
- Correct SID clock-to-host-sample-rate conversion.

## Important architecture rule

The three voices belong to **one emulated SID chip**. They are not three unrelated generic oscillators. The emulator must own the chip register state, clocking, waveform interactions, envelopes and filter. MIDI is a control layer that writes to the chip model; it must not replace the SID's internal architecture.

## Licensing

The SID core must be selected only after its current license and redistribution requirements have been verified. JUCE is dual-licensed; the applicable JUCE license and all SID-core obligations must be respected for the intended distribution. Do not assume a GPL core can be distributed inside a differently licensed closed-source plugin.

## Build

The workflow is in `.github/workflows/build-macos.yml`. It runs on pushes to `main`, pull requests, and manual dispatch. It builds the arm64 VST3 and uploads a ZIP artifact when successful.

## Repository layout

```
GeroSIDSynth/
  CMakeLists.txt
  README.md
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
  .github/workflows/
    build-macos.yml
```

## Current limitation

The current plugin is a silent MIDI-instrument scaffold for validating the VST3 packaging pipeline. The SID emulator and sound generation are the next implementation milestone. No release-quality binary is available yet.
