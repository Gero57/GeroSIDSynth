#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    float parameterValue (juce::AudioProcessorValueTreeState& state, const char* id)
    {
        if (auto* value = state.getRawParameterValue (id))
            return value->load();
        return 0.0f;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout GeroSIDSynthAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using API = juce::AudioParameterInt;
    using APC = juce::AudioParameterChoice;
    using APB = juce::AudioParameterBool;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<APC> ("model", "SID Model",
        juce::StringArray { "MOS 6581", "MOS 8580" }, 0));
    layout.add (std::make_unique<APC> ("waveform", "Waveform",
        juce::StringArray { "Triangle", "Sawtooth", "Pulse", "Noise" }, 1));
    layout.add (std::make_unique<API> ("attack", "Attack", 0, 15, 0));
    layout.add (std::make_unique<API> ("decay", "Decay", 0, 15, 9));
    layout.add (std::make_unique<API> ("sustain", "Sustain", 0, 15, 10));
    layout.add (std::make_unique<API> ("release", "Release", 0, 15, 4));
    layout.add (std::make_unique<APF> ("pulseWidth", "Pulse Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, "%"));
    layout.add (std::make_unique<APB> ("sync", "Sync", false));
    layout.add (std::make_unique<APB> ("ring", "Ring Modulation", false));
    layout.add (std::make_unique<APC> ("filterMode", "Filter Mode",
        juce::StringArray { "Off", "Low Pass", "Band Pass", "High Pass" }, 0));
    layout.add (std::make_unique<API> ("cutoff", "Filter Cutoff", 0, 2047, 1024));
    layout.add (std::make_unique<API> ("resonance", "Resonance", 0, 15, 4));
    return layout;
}

GeroSIDSynthAudioProcessor::GeroSIDSynthAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "GERO_SID_STATE", createParameterLayout())
{
}

void GeroSIDSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    sid = std::make_unique<reSIDfp::residfp>();
    sid->setChipModel (reSIDfp::MOS6581);
    sid->setCombinedWaveforms (reSIDfp::AVERAGE);
    sid->noiseLevel (0);
    sid->setSamplingParameters (sidClockHz, reSIDfp::RESAMPLE, currentSampleRate);
    sid->reset();

    sidSamples.resize (static_cast<size_t> (std::max (samplesPerBlock, 1)));
    voiceNotes.fill (-1);
    releaseSamplesRemaining.fill (0);
    voiceStartOrder.fill (0);
    nextNoteOrder = 1;
    voiceControls.fill (0x20);
    nextVoice = 0;
    lastChipModel = -1;
    applyGlobalParameters();
}

void GeroSIDSynthAudioProcessor::releaseResources()
{
    silenceAllVoices();
    sid.reset();
}

void GeroSIDSynthAudioProcessor::applyGlobalParameters()
{
    if (sid == nullptr)
        return;

    const int model = static_cast<int> (parameterValue (parameters, "model"));
    if (model != lastChipModel)
    {
        sid->setChipModel (model == 0 ? reSIDfp::MOS6581 : reSIDfp::CSG8580);
        lastChipModel = model;
    }

    const int cutoff = juce::jlimit (0, 2047, static_cast<int> (parameterValue (parameters, "cutoff")));
    const int resonance = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "resonance")));
    const int filterMode = static_cast<int> (parameterValue (parameters, "filterMode"));

    sid->write (0x15, static_cast<unsigned char> (cutoff & 0x07));
    sid->write (0x16, static_cast<unsigned char> ((cutoff >> 3) & 0xff));
    sid->write (0x17, static_cast<unsigned char> ((resonance << 4) | (filterMode == 0 ? 0x00 : 0x07)));

    unsigned char modeBits = 0;
    if (filterMode == 1) modeBits = 0x10;
    if (filterMode == 2) modeBits = 0x20;
    if (filterMode == 3) modeBits = 0x40;
    sid->write (0x18, static_cast<unsigned char> (modeBits | 0x0f));

    // Keep waveform and modulation switches responsive for notes already held.
    const int waveform = juce::jlimit (0, 3, static_cast<int> (parameterValue (parameters, "waveform")));
    const unsigned char waveBits[] = { 0x10, 0x20, 0x40, 0x80 };
    for (int voice = 0; voice < sidVoiceCount; ++voice)
    {
        if (voiceNotes[static_cast<size_t> (voice)] < 0)
            continue;
        unsigned char control = static_cast<unsigned char> (waveBits[waveform] | 0x01);
        if (parameterValue (parameters, "sync") >= 0.5f) control |= 0x02;
        if (parameterValue (parameters, "ring") >= 0.5f) control |= 0x04;
        voiceControls[static_cast<size_t> (voice)] = control;
        sid->write (voice * sidRegisterStride + 4, control);
    }
}

void GeroSIDSynthAudioProcessor::renderRange (juce::AudioBuffer<float>& buffer,
                                               int startSample, int numSamples)
{
    if (sid == nullptr || numSamples <= 0)
        return;
    if (sidSamples.size() < static_cast<size_t> (numSamples))
        sidSamples.resize (static_cast<size_t> (numSamples));

    sid->clock (sidSamples.data(), numSamples);
    for (auto& remaining : releaseSamplesRemaining)
        remaining = std::max (0, remaining - numSamples);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float value = static_cast<float> (sidSamples[static_cast<size_t> (sample)]) / 32768.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, startSample + sample, value);
    }
}

void GeroSIDSynthAudioProcessor::startNote (int midiNote, float velocity)
{
    juce::ignoreUnused (velocity);
    if (sid == nullptr || midiNote < 0 || midiNote > 127)
        return;

    int voice = -1;
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
    if (voice < 0)
        voice = nextVoice;

    releaseSamplesRemaining[static_cast<size_t> (voice)] = 0;
    const int base = voice * sidRegisterStride;
    const double frequencyHz = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    const auto frequencyWord = static_cast<unsigned int> (
        std::round (frequencyHz * 16777216.0 / sidClockHz));
    const int pulseWidth = juce::jlimit (0, 4095,
        static_cast<int> (std::round (parameterValue (parameters, "pulseWidth") * 40.95f)));

    voiceControls[static_cast<size_t> (voice)] &= static_cast<unsigned char> (~0x01);
    sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);
    sid->write (base + 0, static_cast<unsigned char> (frequencyWord & 0xff));
    sid->write (base + 1, static_cast<unsigned char> ((frequencyWord >> 8) & 0xff));
    sid->write (base + 2, static_cast<unsigned char> (pulseWidth & 0xff));
    sid->write (base + 3, static_cast<unsigned char> ((pulseWidth >> 8) & 0x0f));

    const int attack = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "attack")));
    const int decay = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "decay")));
    const int sustain = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "sustain")));
    const int release = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "release")));
    sid->write (base + 5, static_cast<unsigned char> ((attack << 4) | decay));
    sid->write (base + 6, static_cast<unsigned char> ((sustain << 4) | release));

    const int waveform = juce::jlimit (0, 3, static_cast<int> (parameterValue (parameters, "waveform")));
    const unsigned char waveBits[] = { 0x10, 0x20, 0x40, 0x80 };
    unsigned char control = static_cast<unsigned char> (waveBits[waveform] | 0x01);
    if (parameterValue (parameters, "sync") >= 0.5f) control |= 0x02;
    if (parameterValue (parameters, "ring") >= 0.5f) control |= 0x04;
    voiceControls[static_cast<size_t> (voice)] = control;
    sid->write (base + 4, control);

    voiceNotes[static_cast<size_t> (voice)] = midiNote;
    voiceStartOrder[static_cast<size_t> (voice)] = nextNoteOrder++;
    nextVoice = (voice + 1) % sidVoiceCount;
}

void GeroSIDSynthAudioProcessor::stopNote (int midiNote)
{
    if (sid == nullptr || midiNote < 0 || midiNote > 127)
        return;

    // Preserve FIFO matching for repeated overlapping pitches.
    int voice = -1;
    auto oldestOrder = std::numeric_limits<unsigned long long>::max();
    for (int candidate = 0; candidate < sidVoiceCount; ++candidate)
        if (voiceNotes[static_cast<size_t> (candidate)] == midiNote
            && voiceStartOrder[static_cast<size_t> (candidate)] < oldestOrder)
        {
            oldestOrder = voiceStartOrder[static_cast<size_t> (candidate)];
            voice = candidate;
        }

    if (voice < 0)
        return;

    // Let the SID envelope enter its natural Release phase. Keep the selected
    // waveform enabled and clear only Gate; hard-muting the control register
    // also kills the envelope and produces an audible click (especially on 6581).
    const int base = voice * sidRegisterStride;
    voiceControls[static_cast<size_t> (voice)] &= static_cast<unsigned char> (~0x01);
    sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);
    voiceNotes[static_cast<size_t> (voice)] = -1;

    // SID release-rate periods in chip cycles for the 4-bit R nibble.
    // Reserve the voice until its envelope has had time to finish releasing.
    static constexpr std::array<int, 16> releaseRateCycles {
        9, 32, 63, 95, 149, 220, 267, 313,
        392, 977, 1954, 3126, 3907, 11720, 19532, 31251
    };
    const int release = juce::jlimit (0, 15,
        static_cast<int> (parameterValue (parameters, "release")));
    const double releaseSeconds = static_cast<double> (releaseRateCycles[static_cast<size_t> (release)])
                                * 256.0 / sidClockHz;
    releaseSamplesRemaining[static_cast<size_t> (voice)] =
        static_cast<int> (std::ceil (currentSampleRate * (releaseSeconds + 0.010)));
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
    if (message.isNoteOn()) startNote (message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff()) stopNote (message.getNoteNumber());
    else if (message.isAllNotesOff()
             || (message.isController() && message.getControllerNumber() == 123))
    {
        // MIDI All Notes Off is an envelope release, not an emergency mute.
        for (int voice = 0; voice < sidVoiceCount; ++voice)
            if (voiceNotes[static_cast<size_t> (voice)] >= 0)
                stopNote (voiceNotes[static_cast<size_t> (voice)]);
    }
    else if (message.isAllSoundOff())
        silenceAllVoices();
}

void GeroSIDSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (sid == nullptr)
    {
        midiMessages.clear();
        return;
    }

    applyGlobalParameters();
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
    return new GeroSIDSynthAudioProcessorEditor (*this);
}

void GeroSIDSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GeroSIDSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GeroSIDSynthAudioProcessor();
}
