#include "PluginEditor.h"

namespace
{
    const juce::Colour bg      (0xff10191d);
    const juce::Colour cabinet (0xff1a292e);
    const juce::Colour panel   (0xff142126);
    const juce::Colour edge    (0xff3c5960);
    const juce::Colour amber   (0xffe2ad57);
    const juce::Colour cream   (0xffe9e6d8);
    const juce::Colour muted   (0xff91a8aa);
    const juce::Colour teal    (0xff5aabb0);
}

SIDLookAndFeel::SIDLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff102027));
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, amber);
    setColour (juce::PopupMenu::backgroundColourId, cabinet);
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, teal.withAlpha (0.35f));
}

void SIDLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider&)
{
    const float diameter = juce::jmin (static_cast<float> (width) - 18.0f,
                                       static_cast<float> (height) - 29.0f);
    const float cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float cy = static_cast<float> (y) + diameter * 0.5f + 3.0f;
    const float radius = diameter * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour (juce::Colour (0xff0a1115));
    g.fillEllipse (cx - radius - 2.0f, cy - radius + 2.0f,
                   diameter + 4.0f, diameter + 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff52666a), cx, cy - radius,
                                             juce::Colour (0xff18262b), cx, cy + radius, false));
    g.fillEllipse (cx - radius, cy - radius, diameter, diameter);
    g.setColour (juce::Colour (0xff71858a));
    g.drawEllipse (cx - radius, cy - radius, diameter, diameter, 1.2f);

    juce::Path track;
    track.addCentredArc (cx, cy, radius - 6.0f, radius - 6.0f, 0.0f,
                         startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff34494e));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (cx, cy, radius - 6.0f, radius - 6.0f, 0.0f,
                            startAngle, angle, true);
    g.setColour (amber);
    g.strokePath (valueArc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    const float pointerLength = radius * 0.62f;
    juce::Path pointer;
    pointer.startNewSubPath (cx, cy);
    pointer.lineTo (cx + std::sin (angle) * pointerLength,
                    cy - std::cos (angle) * pointerLength);
    g.setColour (cream);
    g.strokePath (pointer, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (amber);
    g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

void SIDLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                   int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width) - 1.0f,
                                          static_cast<float> (height) - 1.0f);
    g.setColour (isButtonDown ? juce::Colour (0xff263e44) : juce::Colour (0xff102027));
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    const float cx = static_cast<float> (width) - 17.0f;
    const float cy = static_cast<float> (height) * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
    arrow.lineTo (cx, cy + 2.5f);
    arrow.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (amber);
    g.strokePath (arrow, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SIDLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                        bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    g.setColour (on ? juce::Colour (0xff315b5d) : juce::Colour (0xff101c21));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? teal : edge);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (on ? cream : muted);
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    if (highlighted || down)
    {
        g.setColour (amber.withAlpha (0.12f));
        g.fillRoundedRectangle (r, 5.0f);
    }
}

GeroSIDSynthAudioProcessorEditor::GeroSIDSynthAudioProcessorEditor (GeroSIDSynthAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), sidProcessor (p)
{
    setLookAndFeel (&sidLookAndFeel);
    setSize (1010, 690);

    title.setText ("GERO  /  SID SYNTH", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (29.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, cream);
    addAndMakeVisible (title);

    subtitle.setText ("MOS TECHNOLOGY 6581  •  8580     /     PAL 985.248 Hz", juce::dontSendNotification);
    subtitle.setFont (juce::Font (juce::FontOptions (11.5f)));
    subtitle.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (subtitle);

    for (auto* item : { &envLabel, &filterLabel, &pitchLabel })
    {
        item->setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        item->setColour (juce::Label::textColourId, amber);
        addAndMakeVisible (*item);
    }
    envLabel.setText ("ENVELOPE GENERATOR", juce::dontSendNotification);
    filterLabel.setText ("MULTIMODE FILTER", juce::dontSendNotification);
    pitchLabel.setText ("PITCH  /  PORTAMENTO", juce::dontSendNotification);

    model.addItem ("MOS 6581", 1); model.addItem ("MOS 8580", 2);
    waveform.addItem ("TRIANGLE", 1); waveform.addItem ("SAWTOOTH", 2);
    waveform.addItem ("PULSE", 3); waveform.addItem ("NOISE", 4);
    filterMode.addItem ("OFF", 1); filterMode.addItem ("LOW PASS", 2);
    filterMode.addItem ("BAND PASS", 3); filterMode.addItem ("HIGH PASS", 4);
    octave.addItem ("-2 OCT", 1); octave.addItem ("-1 OCT", 2);
    octave.addItem ("0 OCT", 3); octave.addItem ("+1 OCT", 4); octave.addItem ("+2 OCT", 5);
    voiceMode.addItem ("POLYPHONIC", 1); voiceMode.addItem ("MONOPHONIC", 2);

    for (auto* box : { &model, &waveform, &filterMode, &octave, &voiceMode })
    {
        box->setColour (juce::ComboBox::textColourId, cream);
        box->setColour (juce::ComboBox::outlineColourId, edge);
        box->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*box);
    }

    setupLabel (modelLabel, "CHIP MODEL");
    setupLabel (waveLabel, "OSCILLATOR");
    setupLabel (pulseLabel, "PULSE WIDTH");
    setupLabel (voiceModeLabel, "VOICE MODE");
    setupLabel (cutoffLabel, "CUTOFF");
    setupLabel (resonanceLabel, "RESONANCE");
    setupLabel (attackLabel, "ATTACK");
    setupLabel (decayLabel, "DECAY");
    setupLabel (sustainLabel, "SUSTAIN");
    setupLabel (releaseLabel, "RELEASE");
    setupLabel (octaveLabel, "OCTAVE");
    setupLabel (coarseLabel, "COARSE  /  SEMITONES");
    setupLabel (fineLabel, "FINE  /  CENTS");
    setupLabel (glideLabel, "GLIDE  /  ms");

    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth,
                          &cutoff, &resonance, &coarse, &fine, &glide })
        setupSlider (*slider);

    coarse.setRange (-12.0, 12.0, 1.0);
    fine.setRange (-100.0, 100.0, 0.1);
    glide.setRange (0.0, 2000.0, 1.0);
    addAndMakeVisible (sync);
    addAndMakeVisible (ring);

    auto& state = sidProcessor.parameters;
    modelAttach = std::make_unique<State::ComboBoxAttachment> (state, "model", model);
    waveAttach = std::make_unique<State::ComboBoxAttachment> (state, "waveform", waveform);
    filterAttach = std::make_unique<State::ComboBoxAttachment> (state, "filterMode", filterMode);
    octaveAttach = std::make_unique<State::ComboBoxAttachment> (state, "octave", octave);
    voiceModeAttach = std::make_unique<State::ComboBoxAttachment> (state, "voiceMode", voiceMode);
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
    syncAttach = std::make_unique<State::ButtonAttachment> (state, "sync", sync);
    ringAttach = std::make_unique<State::ButtonAttachment> (state, "ring", ring);
}

GeroSIDSynthAudioProcessorEditor::~GeroSIDSynthAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void GeroSIDSynthAudioProcessorEditor::setupSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 18);
    slider.setColour (juce::Slider::textBoxTextColourId, cream);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0d181c));
    slider.setColour (juce::Slider::textBoxOutlineColourId, edge);
    slider.setColour (juce::Slider::textBoxHighlightColourId, amber);
    addAndMakeVisible (slider);
}

void GeroSIDSynthAudioProcessorEditor::setupLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (label);
}

void GeroSIDSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (cabinet);
    g.fillRoundedRectangle (16.0f, 16.0f, 978.0f, 658.0f, 13.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (16.0f, 16.0f, 978.0f, 658.0f, 13.0f, 1.2f);

    // Thin warm-metal header rule and a small SID-chip insignia.
    g.setColour (amber);
    g.fillRoundedRectangle (36.0f, 103.0f, 938.0f, 2.0f, 1.0f);
    g.setColour (juce::Colour (0xff20383e));
    g.fillRoundedRectangle (36.0f, 122.0f, 938.0f, 143.0f, 8.0f);
    g.fillRoundedRectangle (36.0f, 279.0f, 938.0f, 143.0f, 8.0f);
    g.fillRoundedRectangle (36.0f, 436.0f, 938.0f, 220.0f, 8.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (36.0f, 122.0f, 938.0f, 143.0f, 8.0f, 1.0f);
    g.drawRoundedRectangle (36.0f, 279.0f, 938.0f, 143.0f, 8.0f, 1.0f);
    g.drawRoundedRectangle (36.0f, 436.0f, 938.0f, 220.0f, 8.0f, 1.0f);

    g.setColour (amber);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText ("SID  /  3-VOICE SOUND ENGINE", 730, 49, 230, 18, juce::Justification::centredRight);
    g.setColour (teal);
    g.fillRoundedRectangle (944.0f, 38.0f, 20.0f, 20.0f, 4.0f);
    g.setColour (cream);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("S", 944, 38, 20, 20, juce::Justification::centred);

    g.setColour (edge);
    g.drawLine (520.0f, 454.0f, 520.0f, 638.0f, 1.0f);
}

void GeroSIDSynthAudioProcessorEditor::resized()
{
    title.setBounds (40, 25, 440, 42);
    subtitle.setBounds (42, 68, 520, 20);

    modelLabel.setBounds (54, 137, 125, 17); model.setBounds (54, 160, 130, 34);
    waveLabel.setBounds (205, 137, 135, 17); waveform.setBounds (205, 160, 145, 34);
    pulseLabel.setBounds (368, 137, 105, 17); pulseWidth.setBounds (368, 150, 105, 105);
    sync.setBounds (500, 164, 100, 32); ring.setBounds (612, 164, 128, 32);
    voiceModeLabel.setBounds (785, 137, 150, 17); voiceMode.setBounds (785, 160, 160, 34);

    pitchLabel.setBounds (54, 289, 250, 20);
    octaveLabel.setBounds (58, 333, 110, 17); octave.setBounds (58, 355, 112, 31);
    coarse.setBounds (205, 310, 130, 91); coarseLabel.setBounds (198, 400, 145, 17);
    fine.setBounds (375, 310, 130, 91); fineLabel.setBounds (368, 400, 145, 17);
    glide.setBounds (575, 310, 150, 91); glideLabel.setBounds (578, 400, 145, 17);

    envLabel.setBounds (55, 445, 220, 20);
    filterLabel.setBounds (555, 445, 250, 20);
    juce::Component* envSliders[] = { &attack, &decay, &sustain, &release };
    juce::Component* envLabels[] = { &attackLabel, &decayLabel, &sustainLabel, &releaseLabel };
    for (int i = 0; i < 4; ++i)
    {
        envSliders[i]->setBounds (48 + i * 112, 470, 100, 132);
        envLabels[i]->setBounds (48 + i * 112, 604, 100, 17);
    }
    filterMode.setBounds (555, 486, 145, 34);
    cutoff.setBounds (718, 466, 105, 136); cutoffLabel.setBounds (718, 604, 105, 17);
    resonance.setBounds (846, 466, 105, 136); resonanceLabel.setBounds (846, 604, 105, 17);
}
