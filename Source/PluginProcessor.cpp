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

    // Approximate complete SID envelope-release times for R=0..15.
    // The long settings are important: voices must not be stolen while their
    // audible release tail is still running.
    constexpr std::array<double, 16> sidReleaseSeconds {
        0.006, 0.024, 0.048, 0.072, 0.114, 0.168, 0.204, 0.240,
        0.300, 0.750, 1.500, 2.400, 3.000, 9.000, 15.000, 24.000
    };
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
    voiceNotes.fill (-1);
    voiceControls.fill (0x00);
}

void GeroSIDSynthAudioProcessor::configureSid (reSIDfp::residfp& engine)
{
    engine.setChipModel (parameterValue (parameters, "model") < 0.5f
                         ? reSIDfp::MOS6581 : reSIDfp::CSG8580);
    engine.setCombinedWaveforms (reSIDfp::AVERAGE);
    engine.noiseLevel (0);
    engine.setSamplingParameters (sidClockHz, reSIDfp::RESAMPLE, currentSampleRate);
    engine.reset();
}

void GeroSIDSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;

    for (int voice = 0; voice < maxPolyphony; ++voice)
    {
        sidEngines[static_cast<size_t> (voice)] = std::make_unique<reSIDfp::residfp>();
        configureSid (*sidEngines[static_cast<size_t> (voice)]);
        sidSamples[static_cast<size_t> (voice)].resize (
            static_cast<size_t> (std::max (samplesPerBlock, 1)));
    }

    voiceNotes.fill (-1);
    monoHeldCounts.fill (0);
    monoHeldOrder.fill (0);
    nextMonoOrder = 1;
    releaseSamplesRemaining.fill (0);
    voiceStartOrder.fill (0);
    nextNoteOrder = 1;
    voiceControls.fill (0x00);
    currentFrequencyWords.fill (0.0);
    glideStartWords.fill (0.0);
    glideTargetWords.fill (0.0);
    glideSamplesRemaining.fill (0);
    glideSamplesTotal.fill (0);
    pitchBendSemitones = 0.0;
    vibratoPhase = 0.0;
    modWheelAmount = 0.0;
    nextVoice = 0;
    lastChipModel = -1;
    applyGlobalParameters();
}

void GeroSIDSynthAudioProcessor::releaseResources()
{
    silenceAllVoices();
    for (auto& engine : sidEngines)
        engine.reset();
}

void GeroSIDSynthAudioProcessor::applyGlobalParameters()
{
    const int model = static_cast<int> (parameterValue (parameters, "model"));
    if (model != lastChipModel)
    {
        for (auto& engine : sidEngines)
            if (engine != nullptr)
                engine->setChipModel (model == 0 ? reSIDfp::MOS6581 : reSIDfp::CSG8580);
        lastChipModel = model;
    }

    const int cutoff = juce::jlimit (0, 2047, static_cast<int> (parameterValue (parameters, "cutoff")));
    const int resonance = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "resonance")));
    const int filterMode = static_cast<int> (parameterValue (parameters, "filterMode"));
    unsigned char modeBits = 0;
    if (filterMode == 1) modeBits = 0x10;
    if (filterMode == 2) modeBits = 0x20;
    if (filterMode == 3) modeBits = 0x40;

    const bool syncEnabled = parameterValue (parameters, "sync") >= 0.5f;
    const int waveform = juce::jlimit (0, 3, static_cast<int> (parameterValue (parameters, "waveform")));
    const unsigned char waveBits[] = { 0x10, 0x20, 0x40, 0x80 };

    for (int voice = 0; voice < maxPolyphony; ++voice)
    {
        auto* engine = sidEngines[static_cast<size_t> (voice)].get();
        if (engine == nullptr)
            continue;

        engine->write (0x15, static_cast<unsigned char> (cutoff & 0x07));
        engine->write (0x16, static_cast<unsigned char> ((cutoff >> 3) & 0xff));
        engine->write (0x17, static_cast<unsigned char> ((resonance << 4) | (filterMode == 0 ? 0x00 : 0x07)));
        engine->write (0x18, static_cast<unsigned char> (modeBits | 0x0f));

        if (voiceNotes[static_cast<size_t> (voice)] >= 0)
        {
            unsigned char control = static_cast<unsigned char> (waveBits[waveform] | 0x01);
            if (syncEnabled && voice == 0) control |= 0x02;
            if (parameterValue (parameters, "ring") >= 0.5f) control |= 0x04;
            voiceControls[static_cast<size_t> (voice)] = control;
            engine->write (4, control); // Every polyphonic note uses OSC1 on its own SID.
        }
        else if (releaseSamplesRemaining[static_cast<size_t> (voice)] == 0)
        {
            voiceControls[static_cast<size_t> (voice)] = 0x00;
            engine->write (4, 0x00);
        }
    }

    // SYNC still uses the original SID relationship inside engine zero:
    // OSC1 (played note) is hard-synced by OSC3 (fixed master).
    if (syncEnabled && sidEngines[0] != nullptr)
    {
        const double masterWord = getTunedFrequencyWord (69);
        const auto word = static_cast<unsigned int> (juce::jlimit (0.0, 65535.0, masterWord));
        sidEngines[0]->write (14, static_cast<unsigned char> (word & 0xff));
        sidEngines[0]->write (15, static_cast<unsigned char> ((word >> 8) & 0xff));
        sidEngines[0]->write (18, 0x20);
    }
    else if (sidEngines[0] != nullptr)
    {
        sidEngines[0]->write (18, 0x00);
    }
}

void GeroSIDSynthAudioProcessor::renderRange (juce::AudioBuffer<float>& buffer,
                                               int startSample, int numSamples)
{
    if (numSamples <= 0)
        return;

    int soundingVoices = 0;
    bool hasActiveGlide = false;
    bool hasActiveNotes = false;
    for (int voice = 0; voice < maxPolyphony; ++voice)
    {
        const auto index = static_cast<size_t> (voice);
        const bool sounding = voiceNotes[index] >= 0 || releaseSamplesRemaining[index] > 0;
        if (sounding)
            ++soundingVoices;
        hasActiveGlide = hasActiveGlide || glideSamplesRemaining[index] > 0;
        hasActiveNotes = hasActiveNotes || voiceNotes[index] >= 0;
        if (sounding && sidSamples[index].size() < static_cast<size_t> (numSamples))
            sidSamples[index].resize (static_cast<size_t> (numSamples));
    }

    if (soundingVoices == 0)
        return;

    const double vibratoDepth = juce::jlimit (0.0, 100.0,
        static_cast<double> (parameterValue (parameters, "vibratoDepth")));
    const double vibratoRate = juce::jlimit (0.1, 12.0,
        static_cast<double> (parameterValue (parameters, "vibratoRate")));
    const bool hasActiveVibrato = vibratoDepth > 0.001 && hasActiveNotes;
    const bool updatePerSample = hasActiveGlide || hasActiveVibrato;

    if (updatePerSample)
    {
        const double phaseIncrement = juce::MathConstants<double>::twoPi
                                    * vibratoRate / currentSampleRate;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const double vibratoCents = hasActiveVibrato
                ? std::sin (vibratoPhase) * vibratoDepth * modWheelAmount : 0.0;
            const double vibratoRatio = std::pow (2.0, vibratoCents / 1200.0);

            for (int voice = 0; voice < maxPolyphony; ++voice)
            {
                const auto index = static_cast<size_t> (voice);
                if (voiceNotes[index] < 0 && releaseSamplesRemaining[index] <= 0)
                    continue;

                auto& remaining = glideSamplesRemaining[index];
                if (remaining > 0)
                {
                    const int total = std::max (1, glideSamplesTotal[index]);
                    const double progress = 1.0 - static_cast<double> (remaining) / static_cast<double> (total);
                    const double frequency = glideStartWords[index]
                        + (glideTargetWords[index] - glideStartWords[index]) * progress;
                    currentFrequencyWords[index] = frequency;
                    --remaining;
                    if (remaining == 0)
                        currentFrequencyWords[index] = glideTargetWords[index];
                }

                if (voiceNotes[index] >= 0)
                    setVoiceFrequency (voice, currentFrequencyWords[index] * vibratoRatio);

                sidEngines[index]->clock (&sidSamples[index][static_cast<size_t> (sample)], 1);
            }

            if (hasActiveVibrato)
            {
                vibratoPhase += phaseIncrement;
                if (vibratoPhase >= juce::MathConstants<double>::twoPi)
                    vibratoPhase -= juce::MathConstants<double>::twoPi;
            }
        }
    }
    else
    {
        for (int voice = 0; voice < maxPolyphony; ++voice)
        {
            const auto index = static_cast<size_t> (voice);
            if (voiceNotes[index] >= 0 || releaseSamplesRemaining[index] > 0)
                sidEngines[index]->clock (sidSamples[index].data(), numSamples);
        }
    }

    // Keep every release voice reserved for the complete ADSR tail. This is
    // deliberately independent for each note, so long releases do not cause
    // the allocator to recycle a still-audible oscillator prematurely.
    for (int voice = 0; voice < maxPolyphony; ++voice)
    {
        const auto index = static_cast<size_t> (voice);
        if (releaseSamplesRemaining[index] > 0)
        {
            releaseSamplesRemaining[index] = std::max (0, releaseSamplesRemaining[index] - numSamples);
            if (releaseSamplesRemaining[index] == 0 && voiceNotes[index] < 0)
            {
                voiceControls[index] = 0x00;
                sidEngines[index]->write (4, 0x00);
            }
        }
    }

    const int selectedWaveform = juce::jlimit (0, 3,
        static_cast<int> (parameterValue (parameters, "waveform")));
    const float waveformGain = selectedWaveform == 0 ? 1.25f : 1.0f;
    const float polyphonyGain = 1.0f / std::sqrt (static_cast<float> (std::max (1, soundingVoices)));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mixed = 0.0f;
        for (int voice = 0; voice < maxPolyphony; ++voice)
        {
            const auto index = static_cast<size_t> (voice);
            if (voiceNotes[index] >= 0 || releaseSamplesRemaining[index] > 0)
                mixed += static_cast<float> (sidSamples[index][static_cast<size_t> (sample)]) / 32768.0f;
        }

        const float value = juce::jlimit (-1.0f, 1.0f, mixed * waveformGain * polyphonyGain);
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.addSample (channel, startSample + sample, value);
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
    if (voice < 0 || voice >= maxPolyphony || sidEngines[static_cast<size_t> (voice)] == nullptr)
        return;

    const auto word = static_cast<unsigned int> (juce::jlimit (0.0, 65535.0, frequencyWord));
    auto& engine = *sidEngines[static_cast<size_t> (voice)];
    engine.write (0, static_cast<unsigned char> (word & 0xff));
    engine.write (1, static_cast<unsigned char> ((word >> 8) & 0xff));
}

void GeroSIDSynthAudioProcessor::applyPitchBend (int wheelValue)
{
    const double normalized = (static_cast<double> (juce::jlimit (0, 16383, wheelValue)) - 8192.0) / 8192.0;
    pitchBendSemitones = normalized * 2.0;
    for (int voice = 0; voice < maxPolyphony; ++voice)
    {
        const auto index = static_cast<size_t> (voice);
        if (voiceNotes[index] < 0)
            continue;
        const double target = getTunedFrequencyWord (voiceNotes[index]);
        setVoiceFrequency (voice, target);
        currentFrequencyWords[index] = target;
        glideSamplesRemaining[index] = 0;
    }
}

void GeroSIDSynthAudioProcessor::startNote (int midiNote, float velocity)
{
    juce::ignoreUnused (velocity);
    if (midiNote < 0 || midiNote > 127 || sidEngines[0] == nullptr)
        return;

    const bool syncEnabled = parameterValue (parameters, "sync") >= 0.5f;
    const bool monoMode = parameterValue (parameters, "voiceMode") >= 0.5f || syncEnabled;
    int voice = monoMode ? 0 : -1;

    if (! monoMode)
    {
        // Prefer a genuinely free engine, then the release tail nearest to
        // silence. Only when all 16 notes are held do we steal the oldest.
        for (int offset = 0; offset < maxPolyphony; ++offset)
        {
            const int candidate = (nextVoice + offset) % maxPolyphony;
            const auto index = static_cast<size_t> (candidate);
            if (voiceNotes[index] < 0 && releaseSamplesRemaining[index] == 0)
            {
                voice = candidate;
                break;
            }
        }

        if (voice < 0)
        {
            int shortestRelease = std::numeric_limits<int>::max();
            for (int candidate = 0; candidate < maxPolyphony; ++candidate)
            {
                const auto index = static_cast<size_t> (candidate);
                if (voiceNotes[index] < 0 && releaseSamplesRemaining[index] < shortestRelease)
                {
                    shortestRelease = releaseSamplesRemaining[index];
                    voice = candidate;
                }
            }
        }

        if (voice < 0)
        {
            auto oldestOrder = std::numeric_limits<unsigned long long>::max();
            for (int candidate = 0; candidate < maxPolyphony; ++candidate)
                if (voiceStartOrder[static_cast<size_t> (candidate)] < oldestOrder)
                {
                    oldestOrder = voiceStartOrder[static_cast<size_t> (candidate)];
                    voice = candidate;
                }
        }
    }
    else
    {
        monoHeldCounts[static_cast<size_t> (midiNote)]++;
        monoHeldOrder[static_cast<size_t> (midiNote)] = nextMonoOrder++;
    }

    const auto index = static_cast<size_t> (voice);
    auto& engine = *sidEngines[index];

    // When a slot is stolen, reset only that note's private SID instance.
    // This prevents a previous long envelope or gate state leaking into it.
    if (voiceNotes[index] >= 0 || releaseSamplesRemaining[index] > 0)
    {
        configureSid (engine);
        voiceControls[index] = 0x00;
        applyGlobalParameters();
    }

    releaseSamplesRemaining[index] = 0;
    const double frequencyWord = getTunedFrequencyWord (midiNote);
    const int pulseWidth = juce::jlimit (0, 4095,
        static_cast<int> (std::round (parameterValue (parameters, "pulseWidth") * 40.95f)));

    voiceControls[index] &= static_cast<unsigned char> (~0x01);
    engine.write (4, voiceControls[index]);

    const double glideMs = juce::jlimit (0.0, 2000.0,
        static_cast<double> (parameterValue (parameters, "glide")));
    const int glideSamples = static_cast<int> (std::round (currentSampleRate * glideMs / 1000.0));
    const double previousFrequency = currentFrequencyWords[index];
    if (glideSamples > 0 && previousFrequency > 0.0)
    {
        glideStartWords[index] = previousFrequency;
        glideTargetWords[index] = frequencyWord;
        glideSamplesRemaining[index] = glideSamples;
        glideSamplesTotal[index] = glideSamples;
        setVoiceFrequency (voice, previousFrequency);
    }
    else
    {
        setVoiceFrequency (voice, frequencyWord);
        currentFrequencyWords[index] = frequencyWord;
        glideSamplesRemaining[index] = 0;
        glideSamplesTotal[index] = 0;
    }

    engine.write (2, static_cast<unsigned char> (pulseWidth & 0xff));
    engine.write (3, static_cast<unsigned char> ((pulseWidth >> 8) & 0x0f));

    const int attack = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "attack")));
    const int decay = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "decay")));
    const int sustain = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "sustain")));
    const int release = juce::jlimit (0, 15, static_cast<int> (parameterValue (parameters, "release")));
    engine.write (5, static_cast<unsigned char> ((attack << 4) | decay));
    engine.write (6, static_cast<unsigned char> ((sustain << 4) | release));

    const int waveform = juce::jlimit (0, 3, static_cast<int> (parameterValue (parameters, "waveform")));
    const unsigned char waveBits[] = { 0x10, 0x20, 0x40, 0x80 };
    unsigned char control = static_cast<unsigned char> (waveBits[waveform] | 0x01);
    if (syncEnabled && voice == 0) control |= 0x02;
    if (parameterValue (parameters, "ring") >= 0.5f) control |= 0x04;
    voiceControls[index] = control;
    engine.write (4, control);

    voiceNotes[index] = midiNote;
    voiceStartOrder[index] = nextNoteOrder++;
    nextVoice = (voice + 1) % maxPolyphony;
}

void GeroSIDSynthAudioProcessor::stopNote (int midiNote)
{
    if (midiNote < 0 || midiNote > 127 || sidEngines[0] == nullptr)
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

    // FIFO matching for repeated overlapping pitches.
    int voice = -1;
    auto oldestOrder = std::numeric_limits<unsigned long long>::max();
    for (int candidate = 0; candidate < maxPolyphony; ++candidate)
        if (voiceNotes[static_cast<size_t> (candidate)] == midiNote
            && voiceStartOrder[static_cast<size_t> (candidate)] < oldestOrder)
        {
            oldestOrder = voiceStartOrder[static_cast<size_t> (candidate)];
            voice = candidate;
        }

    if (voice < 0)
        return;

    const auto index = static_cast<size_t> (voice);
    // Clear Gate only: the waveform remains enabled so the SID's own ADSR
    // generator can produce its natural Release tail without a hard click.
    voiceControls[index] &= static_cast<unsigned char> (~0x01);
    sidEngines[index]->write (4, voiceControls[index]);
    voiceNotes[index] = -1;
    glideSamplesRemaining[index] = 0;

    const int release = juce::jlimit (0, 15,
        static_cast<int> (parameterValue (parameters, "release")));
    const double safetyTailSeconds = parameterValue (parameters, "model") < 0.5f ? 0.250 : 0.050;
    releaseSamplesRemaining[index] = static_cast<int> (std::ceil (
        currentSampleRate * (sidReleaseSeconds[static_cast<size_t> (release)] + safetyTailSeconds)));
}

void GeroSIDSynthAudioProcessor::silenceAllVoices()
{
    for (int voice = 0; voice < maxPolyphony; ++voice)
    {
        const auto index = static_cast<size_t> (voice);
        if (sidEngines[index] != nullptr)
            sidEngines[index]->write (4, 0x00);
        voiceControls[index] = 0x00;
        voiceNotes[index] = -1;
        glideSamplesRemaining[index] = 0;
        releaseSamplesRemaining[index] = 0;
    }
    monoHeldCounts.fill (0);
}

void GeroSIDSynthAudioProcessor::handleMidiMessage (const juce::MidiMessage& message)
{
    if (message.isNoteOn())
        startNote (message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        stopNote (message.getNoteNumber());
    else if (message.isPitchWheel())
        applyPitchBend (message.getPitchWheelValue());
    else if (message.isController() && message.getControllerNumber() == 1)
        modWheelAmount = static_cast<double> (message.getControllerValue()) / 127.0;
    else if (message.isAllNotesOff()
             || (message.isController() && message.getControllerNumber() == 123))
    {
        monoHeldCounts.fill (0);
        for (int voice = 0; voice < maxPolyphony; ++voice)
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
    if (sidEngines[0] == nullptr)
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
