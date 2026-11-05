#include "PluginEditor.h"

GeroSIDSynthAudioProcessorEditor::GeroSIDSynthAudioProcessorEditor (GeroSIDSynthAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), sidProcessor (p)
{
    setSize (930, 650);
    title.setText ("GERO  /  SID SYNTH", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, juce::Colour (0xfff0f3ef));
    addAndMakeVisible (title);
    subtitle.setText ("MOS TECHNOLOGY 6581  •  8580     /     PAL 985.248 Hz", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (12.0f)));
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xff91a69d));
    addAndMakeVisible (subtitle);

    for (auto* item : { &envLabel, &filterLabel, &pitchLabel })
    {
        item->setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        item->setColour (juce::Label::textColourId, juce::Colour (0xffe0b45f));
        addAndMakeVisible (*item);
    }
    envLabel.setText ("ENVELOPE", juce::dontSendNotification);
    filterLabel.setText ("FILTER", juce::dontSendNotification);
    pitchLabel.setText ("PITCH  /  PORTAMENTO", juce::dontSendNotification);

    model.addItem ("MOS 6581", 1); model.addItem ("MOS 8580", 2);
    waveform.addItem ("TRIANGLE", 1); waveform.addItem ("SAWTOOTH", 2);
    waveform.addItem ("PULSE", 3); waveform.addItem ("NOISE", 4);
    filterMode.addItem ("OFF", 1); filterMode.addItem ("LOW PASS", 2);
    filterMode.addItem ("BAND PASS", 3); filterMode.addItem ("HIGH PASS", 4);
    octave.addItem ("-2 OCT", 1); octave.addItem ("-1 OCT", 2);
    octave.addItem ("0 OCT", 3); octave.addItem ("+1 OCT", 4); octave.addItem ("+2 OCT", 5);
    for (auto* box : { &model, &waveform, &filterMode, &octave })
    {
        box->setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff202a2a));
        box->setColour (juce::ComboBox::textColourId, juce::Colour (0xfff0f3ef));
        box->setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff52615b));
        addAndMakeVisible (*box);
    }

    setupLabel (modelLabel, "CHIP MODEL"); setupLabel (waveLabel, "WAVEFORM");
    setupLabel (pulseLabel, "PULSE WIDTH"); setupLabel (cutoffLabel, "CUTOFF");
    setupLabel (resonanceLabel, "RESONANCE"); setupLabel (attackLabel, "ATTACK");
    setupLabel (decayLabel, "DECAY"); setupLabel (sustainLabel, "SUSTAIN");
    setupLabel (releaseLabel, "RELEASE"); setupLabel (octaveLabel, "OCTAVE");
    setupLabel (coarseLabel, "COARSE  /  SEMITONES");
    setupLabel (fineLabel, "FINE  /  CENTS");
    setupLabel (glideLabel, "GLIDE  /  ms");

    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth,
                          &cutoff, &resonance, &coarse, &fine, &glide })
        setupSlider (*slider);

    coarse.setRange (-12.0, 12.0, 1.0);
    fine.setRange (-100.0, 100.0, 0.1);
    glide.setRange (0.0, 2000.0, 1.0);
    sync.setColour (juce::ToggleButton::textColourId, juce::Colour (0xfff0f3ef));
    ring.setColour (juce::ToggleButton::textColourId, juce::Colour (0xfff0f3ef));
    addAndMakeVisible (sync); addAndMakeVisible (ring);

    auto& state = sidProcessor.parameters;
    modelAttach = std::make_unique<State::ComboBoxAttachment> (state, "model", model);
    waveAttach = std::make_unique<State::ComboBoxAttachment> (state, "waveform", waveform);
    filterAttach = std::make_unique<State::ComboBoxAttachment> (state, "filterMode", filterMode);
    octaveAttach = std::make_unique<State::ComboBoxAttachment> (state, "octave", octave);
    attackAttach = std::make_unique<State::SliderAttachment> (state, "attack", attack);
    decayAttach = std::make_unique<State::SliderAttachment> (state, "decay", decay);
    sustainAttach = std::make_unique<State::SliderAttachment> (state, "sustain", sustain);
    releaseAttach = std::make_unique<State::SliderAttachment> (state, "release", release);
    pulseAttach = std::make_unique<State::SliderAttachment> (state, "pulseWidth", pulseWidth);
    cutoffAttach = std::make_unique<State::SliderAttachment> (state, "cutoff", cutoff);
    resonanceAttach = std::make_unique<State::SliderAttachment> (state, "resonance", resonance);
    coarseAttach = std::make_unique<State::SliderAttachment> (state, "coarse", coarse);
    fineAttach = std::make_unique<State::SliderAttachment> (state, "fine", fine);
    glideAttach = std::make_unique<State::SliderAttachment> (state, "glide", glide);
}

void GeroSIDSynthAudioProcessorEditor::setupSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 19);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffd8ad58));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff52615b));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xfff0f3ef));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xfff0f3ef));
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff202a2a));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff52615b));
    addAndMakeVisible (slider);
}

void GeroSIDSynthAudioProcessorEditor::setupLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, juce::Colour (0xffaab8b1));
    addAndMakeVisible (label);
}

void GeroSIDSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111817));
    g.setColour (juce::Colour (0xff202a28));
    g.fillRoundedRectangle (16.0f, 16.0f, (float) getWidth() - 32.0f,
                            (float) getHeight() - 32.0f, 14.0f);
    g.setColour (juce::Colour (0xff52615b));
    g.drawRoundedRectangle (16.0f, 16.0f, (float) getWidth() - 32.0f,
                            (float) getHeight() - 32.0f, 14.0f, 1.0f);

    g.setColour (juce::Colour (0xffd8ad58));
    g.fillRoundedRectangle (34.0f, 101.0f, (float) getWidth() - 68.0f, 2.0f, 1.0f);

    const auto panel = [&g] (float x, float y, float w, float h)
    {
        g.setColour (juce::Colour (0xff151e1c));
        g.fillRoundedRectangle (x, y, w, h, 9.0f);
        g.setColour (juce::Colour (0xff3d4b45));
        g.drawRoundedRectangle (x, y, w, h, 9.0f, 1.0f);
    };
    panel (34.0f, 122.0f, 862.0f, 132.0f);
    panel (34.0f, 268.0f, 862.0f, 142.0f);
    panel (34.0f, 424.0f, 862.0f, 194.0f);

    g.setColour (juce::Colour (0xffd8ad58));
    g.drawLine (490.0f, 445.0f, 490.0f, 600.0f, 1.0f);
}

void GeroSIDSynthAudioProcessorEditor::resized()
{
    title.setBounds (38, 25, 440, 42);
    subtitle.setBounds (40, 68, 520, 20);

    modelLabel.setBounds (52, 139, 135, 18); model.setBounds (52, 162, 145, 32);
    waveLabel.setBounds (218, 139, 135, 18); waveform.setBounds (218, 162, 145, 32);
    pulseLabel.setBounds (382, 139, 110, 18); pulseWidth.setBounds (382, 128, 108, 126);
    sync.setBounds (525, 164, 95, 28); ring.setBounds (625, 164, 125, 28);

    pitchLabel.setBounds (52, 278, 230, 20);
    octaveLabel.setBounds (52, 309, 115, 18); octave.setBounds (52, 332, 115, 30);
    coarse.setBounds (195, 292, 130, 100); coarseLabel.setBounds (195, 390, 130, 17);
    fine.setBounds (365, 292, 130, 100); fineLabel.setBounds (365, 390, 130, 17);
    glide.setBounds (555, 292, 150, 100); glideLabel.setBounds (555, 390, 150, 17);

    envLabel.setBounds (52, 432, 130, 20); filterLabel.setBounds (520, 432, 130, 20);
    juce::Component* envSliders[] = { &attack, &decay, &sustain, &release };
    juce::Component* envLabels[] = { &attackLabel, &decayLabel, &sustainLabel, &releaseLabel };
    for (int i = 0; i < 4; ++i)
    {
        envSliders[i]->setBounds (48 + i * 105, 454, 92, 125);
        envLabels[i]->setBounds (48 + i * 105, 582, 92, 18);
    }
    filterMode.setBounds (520, 470, 135, 30);
    cutoff.setBounds (665, 445, 95, 130); cutoffLabel.setBounds (665, 582, 95, 18);
    resonance.setBounds (770, 445, 95, 130); resonanceLabel.setBounds (770, 582, 95, 18);
}
