#pragma once
#include "PluginProcessor.h"

class GeroSIDSynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GeroSIDSynthAudioProcessorEditor (GeroSIDSynthAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using State = juce::AudioProcessorValueTreeState;
    GeroSIDSynthAudioProcessor& sidProcessor;
    juce::ComboBox model, waveform, filterMode, octave;
    juce::Slider attack, decay, sustain, release, pulseWidth, cutoff, resonance, coarse, fine, glide;
    juce::ToggleButton sync { "SYNC" }, ring { "RING MOD" };
    juce::Label title, subtitle, modelLabel, waveLabel, envLabel, filterLabel;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel, pulseLabel, cutoffLabel, resonanceLabel;
    juce::Label pitchLabel, octaveLabel, coarseLabel, fineLabel, glideLabel;
    std::unique_ptr<State::ComboBoxAttachment> modelAttach, waveAttach, filterAttach, octaveAttach;
    std::unique_ptr<State::SliderAttachment> attackAttach, decayAttach, sustainAttach, releaseAttach,
        pulseAttach, cutoffAttach, resonanceAttach, coarseAttach, fineAttach, glideAttach;
    std::unique_ptr<State::ButtonAttachment> syncAttach, ringAttach;
    void setupSlider (juce::Slider&);
    void setupLabel (juce::Label&, const juce::String&);
};
