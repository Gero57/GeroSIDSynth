# Gero SID Synth

A native Apple Silicon VST3 instrument for Cubase, using the cycle-accurate reSIDfp emulation core for the MOS 6581 / MOS 8580 SID.

## Project status

**Stage 2 — SID audio engine integrated; first audible MIDI instrument build in validation.**

The plugin now embeds the upstream reSIDfp engine (pinned to commit `c1791383ca843a855e5d4366da6a7277e34e801d`) and renders its output at the host sample rate. Incoming MIDI notes are assigned to the three voices of one emulated SID chip. The initial voice patch uses sawtooth, a 50% pulse-width setting, and fixed ADSR values. The current editor is still JUCE's generic placeholder; a dedicated Gero interface and the full control set are planned next.

This integration is the first audible milestone, not yet a finished or fully tested SID instrument. The GitHub Actions build must pass and the resulting VST3 must be auditioned in Cubase before sound quality and timing are considered verified.

## Target

- macOS on Apple Silicon (arm64), for the Mac mini M4.
- VST3 instrument for Cubase.
- JUCE 8 + CMake.
- One SID chip instance with its three original independent voices.
- PAL clock: 985,248 Hz (985.248 kHz).
- Selectable 6581 and 8580 models (the current first patch selects 6581).
- MIDI note control for all three SID voices.

## Implemented in this milestone

- reSIDfp cycle-accurate SID core, fetched from a pinned upstream revision.
- PAL clock and host-rate resampling.
- Three-voice MIDI note allocation, including note-off and voice stealing.
- Initial sawtooth patch with fixed ADSR and master volume.
- Stereo plugin output carrying the SID's mono signal on both channels.

## Planned sound and interface features

- Selectable triangle, sawtooth, pulse and noise waveforms.
- Per-voice ADSR controls.
- Pulse-width control.
- Hard sync and ring modulation.
- Selectable MOS 6581 / CSG 8580 models and chip-specific filter behaviour.
- Authentic SID filter controls.
- Dedicated Gero GUI, presets and parameter/state persistence.

## Architecture rule

The three voices belong to **one emulated SID chip**. They are not three unrelated generic oscillators. MIDI is a control layer that writes to the chip's registers; the emulator owns the chip state, clocking, waveform interactions, envelopes and filter.

## Licensing

The SID engine is [reSIDfp](https://github.com/libsidplayfp/libresidfp), distributed under **GNU GPL version 2 or (at your option) any later version**. Its upstream license text is included as `COPYING.residfp`. The exact upstream revision is pinned in `CMakeLists.txt`. When distributing binaries, the corresponding source and applicable license notices must be made available under the relevant GPL terms.

JUCE has its own dual-licensing terms; those terms must also be observed. This repository is public and the build workflow will publish a source bundle alongside the VST3 artifact.

## Build

The workflow is in `.github/workflows/build-macos.yml`. It builds the arm64 VST3 on a macOS runner and uploads both the plugin ZIP and a source archive containing the project and the pinned reSIDfp source.

## Current limitation

The engine is connected to MIDI and audio generation, but this milestone still needs a successful CI build and an in-Cubase listening test. The generic editor exposes no SID controls yet; model selection, waveform selection, filter, ADSR and the custom GUI are subsequent steps.
