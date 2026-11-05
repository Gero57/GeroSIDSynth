#include "PluginEditor.h"

namespace
{
    const juce::Colour bg      (0xffd7d4ca);
    const juce::Colour cabinet (0xffe7e4da);
    const juce::Colour panel   (0xffaaa9a2);
    const juce::Colour edge    (0xff777a78);
    const juce::Colour amber   (0xffe0c34d);
    const juce::Colour cream   (0xfff2f0e8);
    const juce::Colour muted   (0xff555b5d);
    const juce::Colour teal    (0xff668fa4);
}

SIDLookAndFeel::SIDLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff25282a));
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, amber);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff25282a));
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, teal.withAlpha (0.55f));
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

void SIDLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float, float,
                                           juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
        return;

    const float trackW = juce::jmin (18.0f, static_cast<float> (width) * 0.42f);
    const float trackX = static_cast<float> (x) + (static_cast<float> (width) - trackW) * 0.5f;
    const float top = static_cast<float> (y) + 8.0f;
    const float bottom = static_cast<float> (y + height) - 8.0f;
    const float trackH = bottom - top;
    const float fillY = juce::jlimit (top, bottom, sliderPos);

    g.setColour (juce::Colour (0xff282b2d));
    g.fillRoundedRectangle (trackX, top, trackW, trackH, 3.0f);
    g.setColour (juce::Colour (0xffa9c66a));
    g.fillRoundedRectangle (trackX + 1.5f, fillY, trackW - 3.0f, bottom - fillY, 2.0f);
    g.setColour (juce::Colour (0xffe7e3d8));
    g.drawRoundedRectangle (trackX, top, trackW, trackH, 3.0f, 1.0f);

    g.setColour (juce::Colour (0xfff0eee7));
    g.fillRoundedRectangle (trackX - 5.0f, fillY - 3.0f, trackW + 10.0f, 6.0f, 2.0f);
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
    setSize (1100, 740);

    title.setText ("GEROSIDSYNTH", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, cream);
    addAndMakeVisible (title);

    subtitle.setText ("MOS TECHNOLOGY 6581 / 8580     •     PAL SID SOUND ENGINE", juce::dontSendNotification);
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
    setupLabel (vibratoDepthLabel, "VIBRATO  /  CENTS");
    setupLabel (vibratoRateLabel, "VIBRATO  /  Hz");

    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth,
                          &cutoff, &resonance, &coarse, &fine, &glide,
                          &vibratoDepth, &vibratoRate })
        setupSlider (*slider);

    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth })
        slider->setSliderStyle (juce::Slider::LinearVertical);
    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth })
        slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

    coarse.setRange (-12.0, 12.0, 1.0);
    fine.setRange (-100.0, 100.0, 0.1);
    glide.setRange (0.0, 2000.0, 1.0);
    vibratoDepth.setRange (0.0, 100.0, 0.1);
    vibratoRate.setRange (0.1, 12.0, 0.01);
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
    vibratoDepthAttach = std::make_unique<State::SliderAttachment> (state, "vibratoDepth", vibratoDepth);
    vibratoRateAttach = std::make_unique<State::SliderAttachment> (state, "vibratoRate", vibratoRate);
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

    // Warm grey instrument faceplate, inspired by the compact patch-editor layout
    // of classic SID instruments.
    g.setColour (cabinet);
    g.fillRoundedRectangle (12.0f, 12.0f, 1076.0f, 716.0f, 8.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (12.0f, 12.0f, 1076.0f, 716.0f, 8.0f, 1.2f);

    // Black title strip and the characteristic multicolour accent.
    g.setColour (juce::Colour (0xff17191b));
    g.fillRect (24, 24, 1052, 62);
    g.setColour (cream);
    g.setFont (juce::Font (juce::FontOptions (23.0f, juce::Font::bold)));
    g.drawText ("PATCH CREATION", 38, 34, 330, 40, juce::Justification::centredLeft);
    const juce::Colour stripes[] = {
        juce::Colour (0xffdf4d43), juce::Colour (0xffe58b43),
        juce::Colour (0xffe4cf4d), juce::Colour (0xff70a85b),
        juce::Colour (0xff5a9bb4), juce::Colour (0xff7770a8)
    };
    for (int i = 0; i < 6; ++i)
    {
        g.setColour (stripes[i]);
        g.fillRect (390, 35 + i * 6, 666, 4);
    }

    // Three functional areas: oscillator, envelope, filter.
    auto drawModule = [&g] (juce::Rectangle<float> r, const juce::String& name)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (edge);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
        g.setColour (juce::Colour (0xff34383a));
        g.fillRoundedRectanglvoid GeroSIDSynthAudioProcessorEditor::resized()
{
    // Oscillator section: the current engine exposes one shared SID voice setup.
    modelLabel.setBounds (48, 158, 112, 18); model.setBounds (48, 180, 128, 32);
    waveLabel.setBounds (190, 158, 132, 18); waveform.setBounds (190, 180, 142, 32);
    pulseLabel.setBounds (48, 230, 100, 18); pulseWidth.setBounds (52, 250, 52, 132);
    sync.setBounds (126, 260, 88, 30); ring.setBounds (224, 260, 112, 30);
    voiceModeLabel.setBounds (126, 310, 190, 18); voiceMode.setBounds (126, 332, 190, 32);

    // ADSR vertical faders.
    juce::Slider* envSliders[] = { &attack, &decay, &sustain, &release };
    juce::Label* envLabels[] = { &attackLabel, &decayLabel, &sustainLabel, &releaseLabel };
    const char* envNames[] = { "A", "D", "S", "R" };
    for (int i = 0; i < 4; ++i)
    {
        const int x = 416 + i * 72;
        envSliders[i]->setBounds (x, 174, 48, 190);
        envLabels[i]->setText (envNames[i], juce::dontSendNotification);
        envLabels[i]->setBounds (x - 2, 365, 52, 20);
    }
    envLabel.setBounds (0, 0, 1, 1);

    filterMode.setBounds (770, 176, 258, 32);
    cutoffLabel.setBounds (770, 230, 110, 18); cutoff.setBounds (770, 246, 110, 132);
    resonanceLabel.setBounds (906, 230, 110, 18); resonance.setBounds (906, 246, 110, 132);
    filterLabel.setBounds (0, 0, 1, 1);

    // Pitch and modulation controls.
    octaveLabel.setBounds (52, 486, 120, 18); octave.setBounds (52, 510, 120, 32);
    coarseLabel.setBounds (198, 486, 110, 18); coarse.setBounds (194, 508, 112, 132);
    fineLabel.setBounds (342, 486, 110, 18); fine.setBounds (338, 508, 112, 132);
    glideLabel.setBounds (482, 486, 110, 18); glide.setBounds (478, 508, 112, 132);
    vibratoDepthLabel.setBounds (650, 486, 130, 18); vibratoDepth.setBounds (658, 508, 112, 132);
    vibratoRateLabel.setBounds (828, 486, 130, 18); vibratoRate.setBounds (836, 508, 112, 132);
    pitchLabel.setBounds (0, 0, 1, 1);
}include "PluginEditor.h"

namespace
{
    const juce::Colour bg      (0xffd7d4ca);
    const juce::Colour cabinet (0xffe7e4da);
    const juce::Colour panel   (0xffaaa9a2);
    const juce::Colour edge    (0xff777a78);
    const juce::Colour amber   (0xffe0c34d);
    const juce::Colour cream   (0xfff2f0e8);
    const juce::Colour muted   (0xff555b5d);
    const juce::Colour teal    (0xff668fa4);
}

SIDLookAndFeel::SIDLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff25282a));
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, amber);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff25282a));
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, teal.withAlpha (0.55f));
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
    setSize (1100, 740);

    title.setText ("GEROSIDSYNTH", juce::dontSendNotification);
    title.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));
    title.setColour (juce::Label::textColourId, cream);
    addAndMakeVisible (title);

    subtitle.setText ("MOS TECHNOLOGY 6581 / 8580     •     PAL SID SOUND ENGINE", juce::dontSendNotification);
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
    setupLabel (vibratoDepthLabel, "VIBRATO  /  CENTS");
    setupLabel (vibratoRateLabel, "VIBRATO  /  Hz");

    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth,
                          &cutoff, &resonance, &coarse, &fine, &glide,
                          &vibratoDepth, &vibratoRate })
        setupSlider (*slider);

    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth })
        slider->setSliderStyle (juce::Slider::LinearVertical);
    for (auto* slider : { &attack, &decay, &sustain, &release, &pulseWidth })
        slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

    coarse.setRange (-12.0, 12.0, 1.0);
    fine.setRange (-100.0, 100.0, 0.1);
    glide.setRange (0.0, 2000.0, 1.0);
    vibratoDepth.setRange (0.0, 100.0, 0.1);
    vibratoRate.setRange (0.1, 12.0, 0.01);
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
    vibratoDepthAttach = std::make_unique<State::SliderAttachment> (state, "vibratoDepth", vibratoDepth);
    vibratoRateAttach = std::make_unique<State::SliderAttachment> (state, "vibratoRate", vibratoRate);
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

    // Warm grey instrument faceplate, inspired by the compact patch-editor layout
    // of classic SID instruments.
    g.setColour (cabinet);
    g.fillRoundedRectangle (12.0f, 12.0f, 1076.0f, 716.0f, 8.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (12.0f, 12.0f, 1076.0f, 716.0f, 8.0f, 1.2f);

    // Black title strip and the characteristic multicolour accent.
    g.setColour (juce::Colour (0xff17191b));
    g.fillRect (24, 24, 1052, 62);
    g.setColour (cream);
    g.setFont (juce::Font (juce::FontOptions (23.0f, juce::Font::bold)));
    g.drawText ("PATCH CREATION", 38, 34, 330, 40, juce::Justification::centredLeft);
    const juce::Colour stripes[] = {
        juce::Colour (0xffdf4d43), juce::Colour (0xffe58b43),
        juce::Colour (0xffe4cf4d), juce::Colour (0xff70a85b),
        juce::Colour (0xff5a9bb4), juce::Colour (0xff7770a8)
    };
    for (int i = 0; i < 6; ++i)
    {
        g.setColour (stripes[i]);
        g.fillRect (390, 35 + i * 6, 666, 4);
    }

    // Three functional areas: oscillator, envelope, filter.
    auto drawModule = [&g] (juce::Rectangle<float> r, const juce::String& name)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (edge);
        g.drawRoundedRectangle (r, 7.0f, 1.0f);
        g.setColour (juce::Colour (0xff34383a));
        g.fillRoundedRectangle (r.getX() + 8.0f, r.getY() + 8.0f, r.getWidth() - 16.0f, 28.0f, 4.0f);
        g.setColour (cream);
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText (name, r.getX() + 18.0f, r.getY() + 9.0f, r.getWidth() - 36.0f, 26.0f,
                    juce::Justification::centredLeft);
    };
    drawModule ({ 28.0f, 102.0f, 340.0f, 300.0f }, "SID OSCILLATOR");
    drawModule ({ 382.0f, 102.0f, 340.0f, 300.0f }, "ENVELOPE");
    drawModule ({ 736.0f, 102.0f, 336.0f, 300.0f }, "FILTER");
    drawModule ({ 28.0f, 418.0f, 1044.0f, 290.0f }, "PITCH  /  MODULATION");

    // Small hardware-style section captions and dividers.
    g.setColour (juce::Colour (0xff6b7070));
    g.drawLine (48.0f, 150.0f, 348.0f, 150.0f, 1.0f);
    g.drawLine (402.0f, 150.0f, 702.0f, 150.0f, 1.0f);
    g.drawLine (756.0f, 150.0f, 1052.0f, 150.0f, 1.0f);
    g.drawLine (48.0f, 466.0f, 1052.0f, 466.0f, 1.0f);

    // Model badge.
    g.setColour (juce::Colour (0xff34383a));
    g.fillRoundedRectangle (920.0f, 43.0f, 136.0f, 24.0f, 4.0f);
    g.setColour (juce::Colour (0xffa9c6d0));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("MOS 6581 / 8580", 920, 43, 136, 24, juce::Justification::centred);
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
    glide.setBounds (545, 310, 120, 91); glideLabel.setBounds (540, 400, 130, 17);
    vibratoDepth.setBounds (700, 310, 112, 91); vibratoDepthLabel.setBounds (688, 400, 135, 17);
    vibratoRate.setBounds (850, 310, 112, 91); vibratoRateLabel.setBounds (838, 400, 135, 17);

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
