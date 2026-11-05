# Gero SID Synth

A native Apple Silicon VST3 instrument for Cubase, built around a faithful MOS 6581 / MOS 8580 SID emulation.

## Project status

**Stage 0 — repository initialized.** The first milestone is to integrate a legally compatible SID emulation core and establish a repeatable macOS arm64 VST3 build. Sound engine work comes before the custom Gero interface.

## Target

- macOS on Apple Silicon (arm64), initially tested on an M-series Mac.
- VST3 instrument for Cubase.
- JUCE + CMake build.
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

The SID core must be selected only after its current license and redistribution requirements have been verified. The project will retain all required copyright notices, license texts and source-offer obligations. Do not assume that a GPL core can be distributed inside a differently licensed closed-source plugin.

## Build plan

1. Verify the upstream SID emulator, API, license and required source files.
2. Add JUCE and a reproducible CMake VST3 target.
3. Implement and test the SID core independently of the GUI.
4. Add MIDI-to-SID voice allocation and note handling.
5. Add parameter/state persistence and host automation.
6. Build and validate the arm64 VST3 in Cubase.
7. Design the Gero GUI after the sound engine is stable.

## Repository layout (planned)

```
GeroSIDSynth/
  CMakeLists.txt
  README.md
  LICENSES/
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp
    SidEngine/
  Tests/
  .github/workflows/
```

## Current note

No binary plugin is available yet. This repository is the clean project starting point, not a release.
