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
    layout.add (std::make_unique<APC> ("voiceMode", "Voice Mode",
        juce::StringArray { "POLY", "MONO" }, 0));
    layout.add (std::make_unique<APC> ("octave", "Octave",
        juce::StringArray { "-2", "-1", "0", "+1", "+2" }, 2));
    layout.add (std::make_unique<API> ("coarse", "Coarse", -12, 12, 0));
    layout.add (std::make_unique<APF> ("fine", "Fine",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, " cents"));
    layout.add (std::make_unique<APF> ("glide", "Glide",
        juce::NormalisableRange<float> (0.0f, 2000.0f, 1.0f, 0.5f), 0.0f, " ms"));
    layout.add (std::make_unique<APF> ("vibratoDepth", "Vibrato Depth",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, " cents"));
    layout.add (std::make_unique<APF> ("vibratoRate", "Vibrato Rate",
        juce::NormalisableRange<float> (0.1f, 12.0f, 0.01f), 5.0f, " Hz"));
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
    monoHeldCounts.fill (0);
    monoHeldOrder.fill (0);
    nextMonoOrder = 1;
    releaseSamplesRemaining.fill (0);
    voiceStartOrder.fill (0);
    nextNoteOrder = 1;
    voiceControls.fill (0x20);
    currentFrequencyWords.fill (0.0);
    glideStartWords.fill (0.0);
    glideTargetWords.fill (0.0);
    glideSamplesRemaining.fill (0);
    glideSamplesTotal.fill (0);
    pitchBendSemitones = 0.0;
    vibratoPhase = 0.0;
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

    // Hard sync on a real SID is a ring relationship: oscillator 1 is synced
    // by oscillator 3. Keep oscillator 3 running as a fixed master and use
    // oscillator 1 as the played slave. SYNC is therefore a monophonic mode.
    const bool syncEnabled = parameterValue (parameters, "sync") >= 0.5f;
    const int waveform = juce::jlimit (0, 3, static_cast<int> (parameterValue (parameters, "waveform")));
    const unsigned char waveBits[] = { 0x10, 0x20, 0x40, 0x80 };
    for (int voice = 0; voice < sidVoiceCount; ++voice)
    {
        if (voiceNotes[static_cast<size_t> (voice)] < 0)
            continue;
        unsigned char control = static_cast<unsigned char> (waveBits[waveform] | 0x01);
        if (syncEnabled && voice == 0) control |= 0x02;
        if (parameterValue (parameters, "ring") >= 0.5f) control |= 0x04;
        voiceControls[static_cast<size_t> (voice)] = control;
        sid->write (voice * sidRegisterStride + 4, control);
    }
    if (syncEnabled)
    {
        const double masterWord = getTunedFrequencyWord (69);
        setVoiceFrequency (2, masterWord);
        voiceControls[2] = 0x20; // sawtooth, oscillator running, envelope gate low
        sid->write (2 * sidRegisterStride + 4, voiceControls[2]);
    }
    else if (voiceNotes[2] < 0)
    {
        voiceControls[2] = 0x00;
        sid->write (2 * sidRegisterStride + 4, 0x00);
    }
}

void GeroSIDSynthAudioProcessor::renderRange (juce::AudioBuffer<float>& buffer,
                                               int startSample, int numSamples)
{
    if (sid == nullptr || numSamples <= 0)
        return;
    if (sidSamples.size() < static_cast<size_t> (numSamples))
        sidSamples.resize (static_cast<size_t> (numSamples));

    bool hasActiveGlide = false;
    bool hasActiveNotes = false;
    for (int voice = 0; voice < sidVoiceCount; ++voice)
    {
        hasActiveGlide = hasActiveGlide || glideSamplesRemaining[static_cast<size_t> (voice)] > 0;
        hasActiveNotes = hasActiveNotes || voiceNotes[static_cast<size_t> (voice)] >= 0;
    }
    const double vibratoDepth = juce::jlimit (0.0, 100.0,
        static_cast<double> (parameterValue (parameters, "vibratoDepth")));
    const double vibratoRate = juce::jlimit (0.1, 12.0,
        static_cast<double> (parameterValue (parameters, "vibratoRate")));
    // MIDI CC1 (Modulation Wheel) scales the panel Depth control from zero
    // to its configured maximum. Keeping the LFO path alive with Depth set
    // ensures that returning the wheel to zero restores the exact base pitch.
    const bool hasActiveVibrato = vibratoDepth > 0.001 && hasActiveNotes;

    if (hasActiveGlide || hasActiveVibrato)
    {
        // Update SID pitch registers per sample only while glide or vibrato is
        // active. The LFO modulates the tuned frequency, leaving PB independent.
        const double phaseIncrement = juce::MathConstants<double>::twoPi
                                    * vibratoRate / currentSampleRate;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const double vibratoCents = hasActiveVibrato
                ? std::sin (vibratoPhase) * vibratoDepth * modWheelAmount : 0.0;
            const double vibratoRatio = std::pow (2.0, vibratoCents / 1200.0);
            for (int voice = 0; voice < sidVoiceCount; ++voice)
            {
                auto& remaining = glideSamplesRemaining[static_cast<size_t> (voice)];
                if (remaining > 0)
                {
                    const int total = std::max (1, glideSamplesTotal[static_cast<size_t> (voice)]);
                    const double progress = 1.0 - static_cast<double> (remaining) / static_cast<double> (total);
                    const double start = glideStartWords[static_cast<size_t> (voice)];
                    const double target = glideTargetWords[static_cast<size_t> (voice)];
                    const double frequency = start + (target - start) * progress;
                    currentFrequencyWords[static_cast<size_t> (voice)] = frequency;
                    --remaining;
                    if (remaining == 0)
                        currentFrequencyWords[static_cast<size_t> (voice)] = target;
                }
                if (voiceNotes[static_cast<size_t> (voice)] >= 0)
                    setVoiceFrequency (voice, currentFrequencyWords[static_cast<size_t> (voice)] * vibratoRatio);
            }
            if (hasActiveVibrato)
            {
                vibratoPhase += phaseIncrement;
                if (vibratoPhase >= juce::MathConstants<double>::twoPi)
                    vibratoPhase -= juce::MathConstants<double>::twoPi;
            }
            sid->clock (&sidSamples[static_cast<size_t> (sample)], 1);
        }
    }
    else
    {
        sid->clock (sidSamples.data(), numSamples);
    }

    // A SID voice can occasionally keep its oscillator audible after the
    // envelope should have reached zero. Enforce a per-voice release deadline:
    // allow the natural ADSR tail, then disable that oscillator deterministically.
    for (int voice = 0; voice < sidVoiceCount; ++voice)
    {
        auto& remaining = releaseSamplesRemaining[static_cast<size_t> (voice)];
        if (remaining > 0)
        {
            remaining = std::max (0, remaining - numSamples);
            if (remaining == 0 && voiceNotes[static_cast<size_t> (voice)] < 0)
            {
                voiceControls[static_cast<size_t> (voice)] = 0x00;
                sid->write (voice * sidRegisterStride + 4, 0x00);
            }
        }
    }

    // The SID triangle has a noticeably lower perceived output level than
    // the other waveforms. Apply a modest make-up gain without hard clipping.
    const int selectedWaveform = juce::jlimit (0, 3,
        static_cast<int> (parameterValue (parameters, "waveform")));
    const float waveformGain = selectedWaveform == 0 ? 1.25f : 1.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float value = (static_cast<float> (sidSamples[static_cast<size_t> (sample)]) / 32768.0f)
                          * waveformGain;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, startSample + sample, value);
    }
}

double GeroSIDSynthAudioProcessor::getTunedFrequencyWord (int midiNote)
{
    const int octaveIndex = juce::jlimit (0, 4,
        static_cast<int> (parameterValue (parameters, "octave")));
    const int octaveShift = octaveIndex - 2;
    const int coarse = juce::jlimit (-12, 12,
        static_cast<int> (parameterValue (parameters, "coarse")));
    const double fineCents = static_cast<double> (parameterValue (parameters, "fine"));
    const double semitones = static_cast<double> (midiNote + octaveShift * 12 + coarse)
                           + pitchBendSemitones + fineCents / 100.0;
    const double frequencyHz = 440.0 * std::pow (2.0, (semitones - 69.0) / 12.0);
    return juce::jlimit (0.0, 65535.0,
        std::round (frequencyHz * 16777216.0 / sidClockHz));
}

void GeroSIDSynthAudioProcessor::setVoiceFrequency (int voice, double frequencyWord)
{
    if (sid == nullptr || voice < 0 || voice >= sidVoiceCount)
        return;
    const auto word = static_cast<unsigned int> (juce::jlimit (0.0, 65535.0, frequencyWord));
    const int base = voice * sidRegisterStride;
    sid->write (base + 0, static_cast<unsigned char> (word & 0xff));
    sid->write (base + 1, static_cast<unsigned char> ((word >> 8) & 0xff));
}

void GeroSIDSynthAudioProcessor::applyPitchBend (int wheelValue)
{
    const double normalized = (static_cast<double> (juce::jlimit (0, 16383, wheelValue)) - 8192.0) / 8192.0;
    pitchBendSemitones = normalized * 2.0;
    for (int voice = 0; voice < sidVoiceCount; ++voice)
    {
        const int note = voiceNotes[static_cast<size_t> (voice)];
        if (note < 0)
            continue;
        const double target = getTunedFrequencyWord (note);
        setVoiceFrequency (voice, target);
        currentFrequencyWords[static_cast<size_t> (voice)] = target;
        glideSamplesRemaining[static_cast<size_t> (voice)] = 0;
    }
}

void GeroSIDSynthAudioProcessor::startNote (int midiNote, float velocity)
{
    juce::ignoreUnused (velocity);
    if (sid == nullptr || midiNote < 0 || midiNote > 127)
        return;

    const bool syncEnabled = parameterValue (parameters, "sync") >= 0.5f;
    const bool monoMode = parameterValue (parameters, "voiceMode") >= 0.5f || syncEnabled;
    int voice = monoMode ? 0 : -1;
    if (! monoMode)
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
    }
    else
    {
        monoHeldCounts[static_cast<size_t> (midiNote)]++;
        monoHeldOrder[static_cast<size_t> (midiNote)] = nextMonoOrder++;
    }

    releaseSamplesRemaining[static_cast<size_t> (voice)] = 0;
    const int base = voice * sidRegisterStride;
    const double frequencyWord = getTunedFrequencyWord (midiNote);
    const int pulseWidth = juce::jlimit (0, 4095,
        static_cast<int> (std::round (parameterValue (parameters, "pulseWidth") * 40.95f)));

    voiceControls[static_cast<size_t> (voice)] &= static_cast<unsigned char> (~0x01);
    sid->write (base + 4, voiceControls[static_cast<size_t> (voice)]);
    const double glideMs = juce::jlimit (0.0, 2000.0,
        static_cast<double> (parameterValue (parameters, "glide")));
    const int glideSamples = static_cast<int> (std::round (currentSampleRate * glideMs / 1000.0));
    const double previousFrequency = currentFrequencyWords[static_cast<size_t> (voice)];
    if (glideSamples > 0 && previousFrequency > 0.0)
    {
        glideStartWords[static_cast<size_t> (voice)] = previousFrequency;
        glideTargetWords[static_cast<size_t> (voice)] = frequencyWord;
        glideSamplesRemaining[static_cast<size_t> (voice)] = glideSamples;
        glideSamplesTotal[static_cast<size_t> (voice)] = glideSamples;
        setVoiceFrequency (voice, previousFrequency);
    }
    else
    {
        setVoiceFrequency (voice, frequencyWord);
        currentFrequencyWords[static_cast<size_t> (voice)] = frequencyWord;
        glideSamplesRemaining[static_cast<size_t> (voice)] = 0;
        glideSamplesTotal[static_cast<size_t> (voice)] = 0;
    }
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
    if (parameterValue (parameters, "sync") >= 0.5f && voice == 0) control |= 0x02;
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

    if (parameterValue (parameters, "voiceMode") >= 0.5f
        || parameterValue (parameters, "sync") >= 0.5f)
    {
        auto& held = monoHeldCounts[static_cast<size_t> (midiNote)];
        if (held > 0)
            --held;

        if (voiceNotes[0] != midiNote)
            return;

        int fallbackNote = -1;
        unsigned long long newestOrder = 0;
        for (int note = 0; note < 128; ++note)
            if (monoHeldCounts[static_cast<size_t> (note)] > 0
                && monoHeldOrder[static_cast<size_t> (note)] > newestOrder)
            {
                newestOrder = monoHeldOrder[static_cast<size_t> (note)];
                fallbackNote = note;
            }

        if (fallbackNote >= 0)
        {
            const double target = getTunedFrequencyWord (fallbackNote);
            const double glideMs = juce::jlimit (0.0, 2000.0,
                static_cast<double> (parameterValue (parameters, "glide")));
            const int glideSamples = static_cast<int> (std::round (currentSampleRate * glideMs / 1000.0));
            const double previous = currentFrequencyWords[0];
            if (glideSamples > 0 && previous > 0.0)
            {
                glideStartWords[0] = previous;
                glideTargetWords[0] = target;
                glideSamplesRemaining[0] = glideSamples;
                glideSamplesTotal[0] = glideSamples;
            }
            else
            {
                setVoiceFrequency (0, target);
                currentFrequencyWords[0] = target;
                glideSamplesRemaining[0] = 0;
            }
            voiceNotes[0] = fallbackNote;
            voiceStartOrder[0] = nextNoteOrder++;
            return;
        }
    }

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
    glideSamplesRemaining[static_cast<size_t> (voice)] = 0;

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
    // The 6581's analogue-style envelope behaviour can leave a tiny
    // residual at the end of long releases. Give it a little extra time to
    // reach silence before the safety mute; the 8580 keeps the shorter guard.
    const double safetyTailSeconds =
        parameterValue (parameters, "model") < 0.5f ? 0.250 : 0.010;
    releaseSamplesRemaining[static_cast<size_t> (voice)] =
        static_cast<int> (std::ceil (currentSampleRate * (releaseSeconds + safetyTailSeconds)));
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
        glideSamplesRemaining[static_cast<size_t> (voice)] = 0;
        releaseSamplesRemaining[static_cast<size_t> (voice)] =
            static_cast<int> (std::ceil (currentSampleRate * voiceReuseGuardSeconds));
    }
}

void GeroSIDSynthAudioProcessor::handleMidiMessage (const juce::MidiMessage& message)
{
    if (message.isNoteOn()) startNote (message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff()) stopNote (message.getNoteNumber());
    else if (message.isPitchWheel())
        applyPitchBend (message.getPitchWheelValue());
    else if (message.isController() && message.getControllerNumber() == 1)
        modWheelAmount = static_cast<double> (message.getControllerValue()) / 127.0;
    else if (message.isAllNotesOff()
             || (message.isController() && message.getControllerNumber() == 123))
    {
        // MIDI All Notes Off is an envelope release, not an emergency mute.
        monoHeldCounts.fill (0);
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
