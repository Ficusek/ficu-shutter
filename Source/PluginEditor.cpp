#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg      (0xff14161a);
    const juce::Colour kPanel   (0xff1e2128);
    const juce::Colour kAccent  (0xffff7a1a);
    const juce::Colour kText    (0xffe8eaed);
    const juce::Colour kDim     (0xff8a8f98);
}

StutterEditor::StutterEditor (StutterProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setupKnob (stutterKnob, stutterLabel, "STUTTER");
    setupKnob (refreshKnob, refreshLabel, "REFRESH");
    setupKnob (fadeKnob,    fadeLabel,    "FADE");
    setupKnob (mixKnob,     mixLabel,     "MIX");

    for (auto* l : { &stutterValue, &refreshValue })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, kAccent);
        l->setFont (juce::FontOptions (14.0f, juce::Font::bold));
        addAndMakeVisible (*l);
    }

    syncButton.setColour (juce::ToggleButton::textColourId, kText);
    syncButton.setColour (juce::ToggleButton::tickColourId, kAccent);
    addAndMakeVisible (syncButton);

    onButton.setClickingTogglesState (true);
    onButton.setColour (juce::TextButton::buttonColourId, kPanel);
    onButton.setColour (juce::TextButton::buttonOnColourId, kAccent);
    onButton.setColour (juce::TextButton::textColourOffId, kDim);
    onButton.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
    addAndMakeVisible (onButton);

    auto& s = processor.apvts;
    stutterAtt = std::make_unique<SliderAttach> (s, "stutter", stutterKnob);
    refreshAtt = std::make_unique<SliderAttach> (s, "refresh", refreshKnob);
    fadeAtt    = std::make_unique<SliderAttach> (s, "fade",    fadeKnob);
    mixAtt     = std::make_unique<SliderAttach> (s, "mix",     mixKnob);
    syncAtt    = std::make_unique<ButtonAttach> (s, "sync",    syncButton);
    onAtt      = std::make_unique<ButtonAttach> (s, "active",  onButton);

    setSize (480, 270);
    startTimerHz (20);
    timerCallback();
}

StutterEditor::~StutterEditor()
{
    stopTimer();
}

void StutterEditor::setupKnob (juce::Slider& knob, juce::Label& label, const juce::String& name)
{
    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    knob.setColour (juce::Slider::rotarySliderOutlineColourId, kPanel);
    knob.setColour (juce::Slider::thumbColourId, kText);
    addAndMakeVisible (knob);

    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kDim);
    label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (label);
}

void StutterEditor::timerCallback()
{
    stutterValue.setText (processor.getStutterText(), juce::dontSendNotification);
    refreshValue.setText (processor.getRefreshText(), juce::dontSendNotification);
}

void StutterEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setColour (kText);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("FICU STUTTER", 20, 12, 250, 30, juce::Justification::centredLeft);

    g.setColour (kDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("by.ficu", getWidth() - 100, 16, 80, 20, juce::Justification::centredRight);

    g.setColour (kPanel);
    g.fillRoundedRectangle (12.0f, 52.0f, (float) getWidth() - 24.0f, (float) getHeight() - 64.0f, 8.0f);
}

void StutterEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (40);                          // title
    auto bottom = area.removeFromBottom (40);         // buttons row
    area.reduce (8, 8);

    const int colW = area.getWidth() / 4;
    juce::Slider* knobs[]  = { &stutterKnob, &refreshKnob, &fadeKnob, &mixKnob };
    juce::Label*  labels[] = { &stutterLabel, &refreshLabel, &fadeLabel, &mixLabel };

    for (int i = 0; i < 4; ++i)
    {
        auto col = area.removeFromLeft (colW);
        labels[i]->setBounds (col.removeFromTop (18));
        auto valueRow = col.removeFromBottom (20);
        knobs[i]->setBounds (col.reduced (6));
        if (i == 0) stutterValue.setBounds (valueRow);
        if (i == 1) refreshValue.setBounds (valueRow);
    }

    bottom.reduce (20, 4);
    syncButton.setBounds (bottom.removeFromLeft (120));
    onButton.setBounds (bottom.removeFromRight (120));
}
