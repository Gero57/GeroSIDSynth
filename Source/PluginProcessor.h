#pragma once

#include <JuceHeader.h>
#include <residfp/residfp.h>

#include <array>
#include <memory>
#include <vector>

class GeroSIDSynthAudioProcessor final : public juce::AudioProcessor
{
public:
    GeroSIDSynthAudioProcessor();
    ~GeroSIDSynthAudioProcessor() override = default;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 25.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    static constexpr double sidClockHz = 985248.0;
    static constexpr int maxPolyphony = 16;
    static constexpr int sidRegisterStride = 7;

    // A physical SID has three oscillators. To exceed three-note polyphony,
    // each software note gets its own independent SID instance and uses OSC1.
    // This preserves the original SID oscillator/envelope/filter behaviour.
    std::array<std::unique_ptr<reSIDfp::residfp>, maxPolyphony> sidEngines;
    std::array<std::vector<short>, maxPolyphony> sidSamples;
    std::array<int, maxPolyphony> voiceNotes {};
    std::array<int, 128> monoHeldCounts {};
    std::array<unsigned long long, 128> monoHeldOrder {};
    unsigned long long nextMonoOrder = 1;
    std::array<int, maxPolyphony> releaseSamplesRemaining {};
    // Short per-voice fade-in prevents a reset/retrigger transient without
    // changing the SID's own programmable ADSR attack time.
    std::array<int, maxPolyphony> voiceFadeInRemaining {};
    std::array<int, maxPolyphony> voiceFadeInTotal {};
    std::array<unsigned long long, maxPolyphony> voiceStartOrder {};
    unsigned long long nextNoteOrder = 1;
    std::array<unsigned char, maxPolyphony> voiceControls {};
    std::array<double, maxPolyphony> currentFrequencyWords {};
    std::array<double, maxPolyphony> glideStartWords {};
    std::array<double, maxPolyphony> glideTargetWords {};
    std::array<int, maxPolyphony> glideSamplesRemaining {};
    std::array<int, maxPolyphony> glideSamplesTotal {};
    double currentSampleRate = 44100.0;
    double pitchBendSemitones = 0.0;
    double vibratoPhase = 0.0;
    double modWheelAmount = 0.0;
    int nextVoice = 0;
    int lastChipModel = -1;

    void applyGlobalParameters();
    void configureSid (reSIDfp::residfp& engine);
    void renderRange (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void handleMidiMessage (const juce::MidiMessage& message);
    void startNote (int midiNote, float velocity);
    void stopNote (int midiNote);
    void setVoiceFrequency (int voice, double frequencyWord);
    double getTunedFrequencyWord (int midiNote);
    void applyPitchBend (int wheelValue);
    void silenceAllVoices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GeroSIDSynthAudioProcessor)
};
