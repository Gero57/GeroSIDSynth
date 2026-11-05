#include "PluginProcessor.h"

GeroSIDSynthAudioProcessor::GeroSIDSynthAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void GeroSIDSynthAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
}

void GeroSIDSynthAudioProcessor::releaseResources()
{
}

void GeroSIDSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Bootstrap stage only: establish a working MIDI-instrument VST3 target.
    // The temporary silent output will be replaced by the SID core before
    // this project is considered an audible instrument or a test release.
    juce::ignoreUnused (midiMessages, currentSampleRate);
    buffer.clear();
}

juce::AudioProcessorEditor* GeroSIDSynthAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void GeroSIDSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    destData.reset();
}

void GeroSIDSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GeroSIDSynthAudioProcessor();
}
