#include "PluginEditor.h"

GeroSIDSynthAudioProcessorEditor::GeroSIDSynthAudioProcessorEditor (GeroSIDSynthAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), sidProcessor (p)
{
    setSize (820, 520);
    title.setText ("GERO  /  SID SYNTH", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, juce::Colour (0xffe8f0ed));
    addAndMakeVisible (title);
    subtitle.setText ("MOS TECHNOLOGY 6581  •  8580     /     PAL 985.248 Hz", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (12.0f)));
    subtitle.setColour (juce::Label::textColourId, juce::Colour (0xff8da9a2));
    addAndMakeVisible (subtitle);

    for (auto* item : { &envLabel, &filterLabel })
    {
        item->setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        item->setColour (juce::Label::textColourId, juce::Colour (0xff69d6b2));
        addAndMakeVisible (*item);
    }
    envLabel.setText ("ENVELOPE", juce::dontSendNotification);
    filterLabel.setText ("FILTER", juce::dontSendNotification);

    model.addItem ("MOS 6581", 1); model.addItem ("MOS 8580", 2);
    waveform.addItem ("TRIANGLE", 1); waveform.addItem ("SAWTOOTH", 2);
    waveform.addItem ("PULSE", 3); waveform.addItem ("NOISE", 4);
    filterMode.addItem ("OFF", 1); filterMode.addItem ("LOW PASS", 2);
    filterMode.addItem ("BAND PASS", 3); filterMode.addItem ("HIGH PASS", 4);
    for (auto* box : { &model, &waveform, &filterMode })
    {
        box->setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff172824));
        box->setColour (juce::ComboBox::textColourId, juce::Colour (0xffe8f0ed));
        box->setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff35534a));
        addAndMakeVisible (*box);
    }
    setupLabel (modelLabel, "CHIP MODEL"); setupLabel (waveLabel, "WAVEFORM");
    setupLabel (pulseLabel, "PULSE WIDTH"); setupLabel (cutoffLabel, "CUTOFF");
    setupLabel (resonanceLabel, "RESONANCE"); setupLabel (attackLabel, "ATTACK");
    setupLabel (decayLabel, "DECAY"); setupLabel (sustainLabel, "SUSTAIN");
    setupLabel (releaseLabel, "RELEASE");
    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth, &cutoff, &resonance })
        setupSlider (*slider);

    sync.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffe8f0ed));
    ring.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffe8f0ed));
    addAndMakeVisible (sync); addAndMakeVisible (ring);

    auto& s = sidProcessor.parameters;
    modelAttach = std::make_unique<State::ComboBoxAttachment> (s, "model", model);
    waveAttach = std::make_unique<State::ComboBoxAttachment> (s, "waveform", waveform);
    filterAttach = std::make_unique<State::ComboBoxAttachment> (s, "filterMode", filterMode);
    attackAttach = std::make_unique<State::SliderAttachment> (s, "attack", attack);
    decayAttach = std::make_unique<State::SliderAttachment> (s, "decay", decay);
    sustainAttach = std::make_unique<State::SliderAttachment> (s, "sustain", sustain);
    releaseAttach = std::make_unique<State::SliderAttachment> (s, "release", release);
    pulseAttach = std::make_unique<State::SliderAttachment> (s, "pulseWidth", pulseWidth);
    cutoffAttach = std::make_unique<State::SliderAttachment> (s, "cutoff", cutoff);
    resonanceAttach = std::make_unique<State::SliderAttachment> (s, "resonance", resonance);
    syncAttach = std::make_unique<State::ButtonAttachment> (s, "sync", sync);
    ringAttach = std::make_unique<State::ButtonAttachment> (s, "ring", ring);
}

void GeroSIDSynthAudioProcessorEditor::setupSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 19);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff69d6b2));
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff35534a));
    s.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe4b65e));
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe8f0ed));
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff172824));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff35534a));
    addAndMakeVisible (s);
}
void GeroSIDSynthAudioProcessorEditor::setupLabel (juce::Label& l, const juce::String& t)
{
    l.setText (t, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    l.setColour (juce::Label::textColourId, juce::Colour (0xff9bb4ac));
    addAndMakeVisible (l);
}
void GeroSIDSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff101b18));
    g.setColour (juce::Colour (0xff192923));
    g.fillRoundedRectangle (18.0f, 18.0f, (float) getWidth()-36.0f, (float) getHeight()-36.0f, 15.0f);
    g.setColour (juce::Colour (0xff29463d));
    g.drawRoundedRectangle (18.0f, 18.0f, (float) getWidth()-36.0f, (float) getHeight()-36.0f, 15.0f, 1.0f);
    g.setColour (juce::Colour (0xff69d6b2));
    g.fillRoundedRectangle (36.0f, 96.0f, 748.0f, 2.0f, 1.0f);
    g.setColour (juce::Colour (0xff0d1512));
    g.fillRoundedRectangle (36.0f, 126.0f, 748.0f, 132.0f, 9.0f);
    g.fillRoundedRectangle (36.0f, 280.0f, 748.0f, 214.0f, 9.0f);
    g.setColour (juce::Colour (0xff29463d));
    g.drawRoundedRectangle (36.0f, 126.0f, 748.0f, 132.0f, 9.0f, 1.0f);
    g.drawRoundedRectangle (36.0f, 280.0f, 748.0f, 214.0f, 9.0f, 1.0f);
}
void GeroSIDSynthAudioProcessorEditor::resized()
{
    title.setBounds (40, 27, 440, 42); subtitle.setBounds (42, 68, 500, 20);
    modelLabel.setBounds (52, 145, 120, 18); model.setBounds (52, 168, 145, 32);
    waveLabel.setBounds (220, 145, 120, 18); waveform.setBounds (220, 168, 145, 32);
    pulseLabel.setBounds (390, 145, 110, 18); pulseWidth.setBounds (392, 119, 108, 136);
    sync.setBounds (535, 172, 95, 28); ring.setBounds (640, 172, 125, 28);
    envLabel.setBounds (52, 288, 130, 20); filterLabel.setBounds (470, 288, 130, 20);
    juce::Component* envSliders[] = { &attack, &decay, &sustain, &release };
    juce::Component* envLabels[] = { &attackLabel, &decayLabel, &sustainLabel, &releaseLabel };
    for (int i=0; i<4; ++i)
    {
        envSliders[i]->setBounds (48+i*98, 318, 88, 125);
        envLabels[i]->setBounds (48+i*98, 461, 88, 18);
    }
    filterMode.setBounds (470, 342, 122, 30);
    cutoff.setBounds (600, 316, 86, 126); cutoffLabel.setBounds (600, 461, 86, 18);
    resonance.setBounds (692, 316, 86, 126); resonanceLabel.setBounds (692, 461, 86, 18);
}
