#pragma once
#include "PluginProcessor.h"

class StutterEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit StutterEditor (StutterProcessor&);
    ~StutterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, juce::Label&, const juce::String& name);

    StutterProcessor& processor;

    juce::Slider stutterKnob, refreshKnob, fadeKnob, mixKnob;
    juce::Label stutterLabel, refreshLabel, fadeLabel, mixLabel;
    juce::Label stutterValue, refreshValue;
    juce::ToggleButton syncButton { "SYNC" };
    juce::TextButton onButton { "ON" };

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttach> stutterAtt, refreshAtt, fadeAtt, mixAtt;
    std::unique_ptr<ButtonAttach> syncAtt, onAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterEditor)
};
