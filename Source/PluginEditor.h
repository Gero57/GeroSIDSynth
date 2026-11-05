#pragma once
#include "PluginProcessor.h"

class SIDLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SIDLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
};

class GeroSIDSynthAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GeroSIDSynthAudioProcessorEditor (GeroSIDSynthAudioProcessor&);
    ~GeroSIDSynthAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using State = juce::AudioProcessorValueTreeState;
    GeroSIDSynthAudioProcessor& sidProcessor;
    SIDLookAndFeel sidLookAndFeel;
    juce::ComboBox model, waveform, filterMode, octave, voiceMode;
    juce::Slider attack, decay, sustain, release, pulseWidth, cutoff, resonance, coarse, fine, glide,
        vibratoDepth, vibratoRate, vibratoDelay;
    juce::ToggleButton sync { "SYNC" }, ring { "RING MOD" };
    juce::Label title, subtitle, modelLabel, waveLabel, envLabel, filterLabel;
    juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel, pulseLabel, cutoffLabel, resonanceLabel;
    juce::Label pitchLabel, octaveLabel, coarseLabel, fineLabel, glideLabel, voiceModeLabel,
        vibratoDepthLabel, vibratoRateLabel, vibratoDelayLabel;
    std::unique_ptr<State::ComboBoxAttachment> modelAttach, waveAttach, filterAttach, octaveAttach, voiceModeAttach;
    std::unique_ptr<State::SliderAttachment> attackAttach, decayAttach, sustainAttach, releaseAttach,
        pulseAttach, cutoffAttach, resonanceAttach, coarseAttach, fineAttach, glideAttach,
        vibratoDepthAttach, vibratoRateAttach, vibratoDelayAttach;
    std::unique_ptr<State::ButtonAttachment> syncAttach, ringAttach;
    void setupSlider (juce::Slider&);
    void setupLabel (juce::Label&, const juce::String&);
};
