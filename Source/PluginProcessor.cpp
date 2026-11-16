#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Stutter slice lengths in beats (quarter notes)
    const double kStutterBeats[] = { 0.125, 0.25, 1.0 / 3.0, 0.5, 2.0 / 3.0, 1.0, 2.0, 4.0 };
    const char* kStutterNames[]  = { "1/32", "1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2", "1 bar" };
    constexpr int kNumStutter = 8;

    // Refresh intervals in beats
    const double kRefreshBeats[] = { 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 };
    const char* kRefreshNames[]  = { "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars" };
    constexpr int kNumRefresh = 6;

    constexpr double kMaxSliceSeconds = 6.0;
}

StutterProcessor::StutterProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pStutter = apvts.getRawParameterValue ("stutter");
    pRefresh = apvts.getRawParameterValue ("refresh");
    pFade    = apvts.getRawParameterValue ("fade");
    pMix     = apvts.getRawParameterValue ("mix");
    pSync    = apvts.getRawParameterValue ("sync");
    pActive  = apvts.getRawParameterValue ("active");
}

juce::AudioProcessorValueTreeState::ParameterLayout StutterProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "stutter", 1 }, "Stutter",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "refresh", 1 }, "Refresh",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "fade", 1 }, "Fade",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "Sync", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "active", 1 }, "On", false));
    return layout;
}

int StutterProcessor::stutterIndex (float v) const
{
    return juce::jlimit (0, kNumStutter - 1, (int) std::round (v * (kNumStutter - 1)));
}

int StutterProcessor::refreshIndex (float v) const
{
    return juce::jlimit (0, kNumRefresh - 1, (int) std::round (v * (kNumRefresh - 1)));
}

double StutterProcessor::stutterSamples (float v, bool sync, double bpm) const
{
    const double maxLen = kMaxSliceSeconds * currentSampleRate;
    double len;
    if (sync)
        len = kStutterBeats[stutterIndex (v)] * 60.0 / bpm * currentSampleRate;
    else
        len = 0.010 * std::pow (50.0, (double) v) * currentSampleRate;   // 10 ms .. 500 ms

    return juce::jlimit (64.0, maxLen, len);
}

double StutterProcessor::refreshSamples (float v, bool sync, double bpm) const
{
    double len;
    if (sync)
        len = kRefreshBeats[refreshIndex (v)] * 60.0 / bpm * currentSampleRate;
    else
        len = 0.050 * std::pow (40.0, (double) v) * currentSampleRate;   // 50 ms .. 2 s

    return juce::jmax (64.0, len);
}

juce::String StutterProcessor::getStutterText() const
{
    const float v = pStutter->load();
    if (pSync->load() > 0.5f)
        return kStutterNames[stutterIndex (v)];
    return juce::String (juce::roundToInt (10.0 * std::pow (50.0, (double) v))) + " ms";
}

juce::String StutterProcessor::getRefreshText() const
{
    const float v = pRefresh->load();
    if (pSync->load() > 0.5f)
        return kRefreshNames[refreshIndex (v)];
    return juce::String (juce::roundToInt (50.0 * std::pow (40.0, (double) v))) + " ms";
}

void StutterProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    ringSize = (int) (kMaxSliceSeconds * sampleRate) + 8;

    ring.setSize  (2, ringSize);
    slice.setSize (2, ringSize);
    ring.clear();
    slice.clear();

    writePos = 0;
    playPos = 0;
    sliceLen = 64;
    refreshCounter = 0;
    forceCapture = true;
    wasActive = false;

    activeGain.reset (sampleRate, 0.005);
    activeGain.setCurrentAndTargetValue (0.0f);
}

bool StutterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void StutterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // Tempo from host
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 20.0)
                    bpm = *b;
    lastBpm.store (bpm);

    const bool sync   = pSync->load() > 0.5f;
    const bool active = pActive->load() > 0.5f;
    const float mix   = pMix->load();

    const int targetLen = (int) stutterSamples (pStutter->load(), sync, bpm);
    const int refreshLen = (int) refreshSamples (pRefresh->load(), sync, bpm);

    activeGain.setTargetValue (active ? 1.0f : 0.0f);

    if (active && ! wasActive)
        forceCapture = true;
    wasActive = active;

    for (int n = 0; n < numSamples; ++n)
    {
        // 1) record input
        float dry[2] = { 0.0f, 0.0f };
        for (int ch = 0; ch < numCh; ++ch)
        {
            dry[ch] = buffer.getSample (ch, n);
            ring.setSample (ch, writePos, dry[ch]);
        }
        writePos = (writePos + 1) % ringSize;

        const float a = activeGain.getNextValue();

        if (a > 0.0f || active)
        {
            // 2) (re)capture the loop from the latest audio
            if (forceCapture || refreshCounter >= refreshLen)
            {
                sliceLen = targetLen;
                int start = writePos - sliceLen;
                if (start < 0)
                    start += ringSize;

                for (int ch = 0; ch < numCh; ++ch)
                    for (int i = 0; i < sliceLen; ++i)
                        slice.setSample (ch, i, ring.getSample (ch, (start + i) % ringSize));

                playPos = 0;
                refreshCounter = 0;
                forceCapture = false;
            }

            // 3) fade at the edges of every repeat (avoids clicks)
            const int fadeSamples = juce::jlimit (16, juce::jmax (16, sliceLen / 2),
                                                  (int) (pFade->load() * (float) sliceLen * 0.5f));
            float env = 1.0f;
            if (playPos < fadeSamples)
                env = (float) playPos / (float) fadeSamples;
            const int fromEnd = sliceLen - 1 - playPos;
            if (fromEnd < fadeSamples)
                env = juce::jmin (env, (float) fromEnd / (float) fadeSamples);

            // 4) output = dry/wet blend
            const float wetAmt = mix * a;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float wet = slice.getSample (ch, playPos) * env;
                buffer.setSample (ch, n, dry[ch] * (1.0f - wetAmt) + wet * wetAmt);
            }

            if (++playPos >= sliceLen)
                playPos = 0;
            ++refreshCounter;
        }
    }
}

juce::AudioProcessorEditor* StutterProcessor::createEditor()
{
    return new StutterEditor (*this);
}

void StutterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void StutterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StutterProcessor();
}
