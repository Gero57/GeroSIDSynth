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

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.025; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    static constexpr double sidClockHz = 985248.0;
    static constexpr int sidVoiceCount = 3;
    static constexpr int sidRegisterStride = 7;
    static constexpr double voiceReuseGuardSeconds = 0.020;

    std::unique_ptr<reSIDfp::residfp> sid;
    std::vector<short> sidSamples;
    std::array<int, sidVoiceCount> voiceNotes { -1, -1, -1 };
    std::array<int, sidVoiceCount> releaseSamplesRemaining { 0, 0, 0 };
    std::array<unsigned long long, sidVoiceCount> voiceStartOrder { 0, 0, 0 };
    unsigned long long nextNoteOrder = 1;
    std::array<unsigned char, sidVoiceCount> voiceControls { 0x20, 0x20, 0x20 };
    double currentSampleRate = 44100.0;
    int nextVoice = 0;

    void renderRange (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void handleMidiMessage (const juce::MidiMessage& message);
    void startNote (int midiNote, float velocity);
    void stopNote (int midiNote);
    void silenceAllVoices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GeroSIDSynthAudioProcessor)
};
