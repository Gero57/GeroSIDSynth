#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>

GeroSIDSynthAudioProcessor::GeroSIDSynthAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    noteVoices.fill (-1);
}

void GeroSIDSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sid = std::make_unique<reSIDfp::residfp>();
    sid->setChipModel (reSIDfp::MOS6581);
    sid->setCombinedWaveforms (reSIDfp::AVERAGE);
    sid->setSamplingParameters (sidClockHz, reSIDfp::RESAMPLE, sampleRate);
    sid->reset();

    sidSamples.resize (static_cast<size_t> (std::max (samplesPerBlock, 1)));
    voiceNotes.fill (-1);
    noteVoices.fill (-1);
    voiceControls.fill (0x20);
    nextVoice = 0;

    // Master volume register. All three voices are mixed by this single SID.
    sid->write (0x18, 0x0f);
}

void GeroSIDSynthAudioProcessor::releaseResources()
{
    silenceAllVoices();
    sid.reset();
}

void GeroSIDSynthAudioProcessor::renderRange (juce::AudioBuffer<float>& buffer,
                                               int startSample,
                                               int numSamples)
{
    if (sid == nullptr || numSamples <= 0)
        return;

    if (sidSamples.size() < static_cast<size_t> (numSamples))
        sidSamples.resize (static_cast<size_t> (numSamples));

    // reSIDfp clocks the chip and resamples directly to the host sample rate.
    sid->clock (sidSamples.data(), numSamples);

    const int channels = buffer.getNumChannels();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float value = static_cast<float> (sidSamples[static_cast<size_t> (sample)])
                          * (1.0f / 32768.0f);

        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample (channel, startSample + sample, value);
    }
}

void GeroSIDSynthAudioProcessor::startNote (int midiNote, float velocity)
{
    juce::ignoreUnused (velocity);

    if (sid == nullptr || midiNote < 0 || midiNote > 127)
        return;

    int voice = noteVoices[static_cast<size_t> (midiNote)];
    if (voice < 0)
    {
        for (int offset = 0; offset < sidVoiceCount; ++offset)
        {
            const int candidate = (nextVoice + offset) % sidVoiceCount;
            if (voiceNotes[static_cast<size_t> (candidate)] < 0)
            {
                voice = candidate;
                break;
            }
        }
    }

    // If all three SID oscillators are busy, steal the next voice in rotation.
    if (voice < 0)
        voice = nextVoice;

    const int previousNote = voiceNotes[static_cast<size_t> (voice)];
    if (previousNote >= 0)
        noteVoices[static_cast<size_t> (previousNote)] = -1;

    const int base = voice * sidRegisterStride;
    const double frequencyHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    const auto frequencyWord = static_cast<unsigned int> (
        std::round (frequencyHz * 16777216.0 / sidClockHz));
    const unsigned int pulseWidth = 2048; // 50% pulse width; available for later GUI control.

    sid->write (base + 0, static_cast<unsigned char> (frequencyWord & 0xff));
    sid->write (base + 1, static_cast<unsigned char> ((frequencyWord >> 8) & 0xff));
    sid->write (base + 2, static_cast<unsigned char> (pulseWidth & 0xff));
    sid->write (base + 3, static_cast<unsigned char> ((pulseWidth >> 8) & 0x0f));
    sid->write (base + 5, 0x09); // Attack 0 / Decay 9
    sid->write (base + 6, 0xa8); // Sustain 10 / Release 8

    voiceControls[static_cast<size_t> (voice)] = 0x21; // Sawtooth + gate.
    sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);

    voiceNotes[static_cast<size_t> (voice)] = midiNote;
    noteVoices[static_cast<size_t> (midiNote)] = voice;
    nextVoice = (voice + 1) % sidVoiceCount;
}

void GeroSIDSynthAudioProcessor::stopNote (int midiNote)
{
    if (sid == nullptr || midiNote < 0 || midiNote > 127)
        return;

    const int voice = noteVoices[static_cast<size_t> (midiNote)];
    if (voice < 0)
        return;

    const int base = voice * sidRegisterStride;
    voiceControls[static_cast<size_t> (voice)] &= static_cast<unsigned char> (~0x01);
    sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);
    voiceNotes[static_cast<size_t> (voice)] = -1;
    noteVoices[static_cast<size_t> (midiNote)] = -1;
}

void GeroSIDSynthAudioProcessor::silenceAllVoices()
{
    if (sid == nullptr)
        return;

    for (int voice = 0; voice < sidVoiceCount; ++voice)
    {
        const int base = voice * sidRegisterStride;
        voiceControls[static_cast<size_t> (voice)] &= static_cast<unsigned char> (~0x01);
        sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);
        voiceNotes[static_cast<size_t> (voice)] = -1;
    }

    noteVoices.fill (-1);
}

void GeroSIDSynthAudioProcessor::handleMidiMessage (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
        startNote (message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        stopNote (message.getNoteNumber());
    else if (message.isAllNotesOff() || message.isAllSoundOff())
        silenceAllVoices();
    else if (message.isController() && message.getControllerNumber() == 123)
        silenceAllVoices();
}

void GeroSIDSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (sid == nullptr)
    {
        midiMessages.clear();
        return;
    }

    int renderedUntil = 0;
    juce::MidiBuffer::Iterator iterator (midiMessages);
    juce::MidiMessage message;
    int eventSample = 0;

    while (iterator.getNextEvent (message, eventSample))
    {
        const int safePosition = juce::jlimit (renderedUntil, buffer.getNumSamples(), eventSample);
        renderRange (buffer, renderedUntil, safePosition - renderedUntil);
        renderedUntil = safePosition;
        handleMidiMessage (message);
    }

    renderRange (buffer, renderedUntil, buffer.getNumSamples() - renderedUntil);
    midiMessages.clear();
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
