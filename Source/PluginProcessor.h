#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class StutterProcessor : public juce::AudioProcessor
{
public:
    StutterProcessor();
    ~StutterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ficu Stutter"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Text shown under the knobs in the editor
    juce::String getStutterText() const;
    juce::String getRefreshText() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Parameter -> time helpers
    int stutterIndex (float v) const;
    int refreshIndex (float v) const;
    double stutterSamples (float v, bool sync, double bpm) const;
    double refreshSamples (float v, bool sync, double bpm) const;

    std::atomic<float>* pStutter = nullptr;
    std::atomic<float>* pRefresh = nullptr;
    std::atomic<float>* pFade    = nullptr;
    std::atomic<float>* pMix     = nullptr;
    std::atomic<float>* pSync    = nullptr;
    std::atomic<float>* pActive  = nullptr;

    // Audio state
    juce::AudioBuffer<float> ring;    // continuously recorded input
    juce::AudioBuffer<float> slice;   // the captured loop that gets repeated
    int ringSize = 0;
    int writePos = 0;
    int sliceLen = 1;
    int playPos = 0;
    int refreshCounter = 0;
    bool forceCapture = true;
    bool wasActive = false;

    juce::SmoothedValue<float> activeGain;

    double currentSampleRate = 44100.0;
    std::atomic<double> lastBpm { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StutterProcessor)
};
