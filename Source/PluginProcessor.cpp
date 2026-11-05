#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <limits>

GeroSIDSynthAudioProcessor::GeroSIDSynthAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void GeroSIDSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    sid = std::make_unique<reSIDfp::residfp>();
    sid->setChipModel (reSIDfp::MOS6581);
    sid->setCombinedWaveforms (reSIDfp::AVERAGE);
    // Keep the emulator's synthetic background-noise floor disabled.
    // The SID's actual per-voice Noise waveform remains available.
    sid->noiseLevel (0);
    sid->setSamplingParameters (sidClockHz, reSIDfp::RESAMPLE, currentSampleRate);
    sid->reset();

    sidSamples.resize (static_cast<size_t> (std::max (samplesPerBlock, 1)));
    voiceNotes.fill (-1);
    releaseSamplesRemaining.fill (0);
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

    // A released SID voice remains unavailable briefly so a new note cannot
    // inherit the previous note's still-decaying envelope.
    for (auto& remaining : releaseSamplesRemaining)
        remaining = std::max (0, remaining - numSamples);

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

    int voice = -1;

    // Prefer a genuinely idle voice. A voice in its short release guard period
    // is not considered free yet, even though its MIDI note has ended.
    if (voice < 0)
    {
        for (int offset = 0; offset < sidVoiceCount; ++offset)
        {
            const int candidate = (nextVoice + offset) % sidVoiceCount;
            if (voiceNotes[static_cast<size_t> (candidate)] < 0
                && releaseSamplesRemaining[static_cast<size_t> (candidate)] == 0)
            {
                voice = candidate;
                break;
            }
        }
    }

    // If all voices are occupied, first steal the release tail closest to silence.
    if (voice < 0)
    {
        int shortestRelease = std::numeric_limits<int>::max();
        for (int candidate = 0; candidate < sidVoiceCount; ++candidate)
        {
            const int remaining = releaseSamplesRemaining[static_cast<size_t> (candidate)];
            if (voiceNotes[static_cast<size_t> (candidate)] < 0 && remaining < shortestRelease)
            {
                shortestRelease = remaining;
                voice = candidate;
            }
        }
    }

    // If all three notes are held, steal the next voice in rotation.
    if (voice < 0)
        voice = nextVoice;

    releaseSamplesRemaining[static_cast<size_t> (voice)] = 0;

    const int base = voice * sidRegisterStride;
    const double frequencyHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    const auto frequencyWord = static_cast<unsigned int> (
        std::round (frequencyHz * 16777216.0 / sidClockHz));
    const unsigned int pulseWidth = 2048; // 50% pulse width; available for later GUI control.

    // Ensure gate is low before programming and retriggering this oscillator.
    voiceControls[static_cast<size_t> (voice)] &= static_cast<unsigned char> (~0x01);
    sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);

    sid->write (base + 0, static_cast<unsigned char> (frequencyWord & 0xff));
    sid->write (base + 1, static_cast<unsigned char> ((frequencyWord >> 8) & 0xff));
    sid->write (base + 2, static_cast<unsigned char> (pulseWidth & 0xff));
    sid->write (base + 3, static_cast<unsigned char> ((pulseWidth >> 8) & 0x0f));
    sid->write (base + 5, 0x09); // Attack 0 / Decay 9
    sid->write (base + 6, 0xa0); // Sustain 10 / Release 0: shortest SID release.

    voiceControls[static_cast<size_t> (voice)] = 0x21; // Sawtooth + gate.
    sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);

    voiceNotes[static_cast<size_t> (voice)] = midiNote;
    voiceStartOrder[static_cast<size_t> (voice)] = nextNoteOrder++;
    nextVoice = (voice + 1) % sidVoiceCount;
}

void GeroSIDSynthAudioProcessor::stopNote (int midiNote)
{
    if (sid == nullptr || midiNote < 0 || midiNote > 127)
        return;

    // A single MIDI pitch may have multiple overlapping Note Ons.
    // Find the oldest still-held instance (FIFO Note Off handling) instead
    // of keeping one pitch->voice mapping that gets overwritten on retrigger.
    int voice = -1;
    auto oldestOrder = std::numeric_limits<unsigned long long>::max();
    for (int candidate = 0; candidate < sidVoiceCount; ++candidate)
    {
        if (voiceNotes[static_cast<size_t> (candidate)] == midiNote
            && voiceStartOrder[static_cast<size_t> (candidate)] < oldestOrder)
        {
            oldestOrder = voiceStartOrder[static_cast<size_t> (candidate)];
            voice = candidate;
        }
    }
    if (voice < 0)
        return;

    const int base = voice * sidRegisterStride;
    // Diagnostic hard mute: clear the waveform as well as Gate so a voice
    // cannot leave a sustained oscillator audible after MIDI Note Off.
    voiceControls[static_cast<size_t> (voice)] = 0x00;
    sid->write (base + 4, 0x00);

    voiceNotes[static_cast<size_t> (voice)] = -1;
    releaseSamplesRemaining[static_cast<size_t> (voice)] =
        static_cast<int> (std::ceil (currentSampleRate * voiceReuseGuardSeconds));
}

void GeroSIDSynthAudioProcessor::silenceAllVoices()
{
    if (sid == nullptr)
        return;

    for (int voice = 0; voice < sidVoiceCount; ++voice)
    {
        const int base = voice * sidRegisterStride;
        voiceControls[static_cast<size_t> (voice)] = 0x00;
        sid->write (base + 4, 0x00);
        voiceNotes[static_cast<size_t> (voice)] = -1;
        releaseSamplesRemaining[static_cast<size_t> (voice)] =
            static_cast<int> (std::ceil (currentSampleRate * voiceReuseGuardSeconds));
    }

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
