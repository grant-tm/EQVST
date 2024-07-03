/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// contains parameters for peak, low cut, and high cut filters
struct ChainSettings
{
    // peak parameters
    float peakFreq{ 0 };
    float peakGainDB{ 0 };
    float peakQ{ 1.f };

    // low cut parameters
    float lowCutFreq{ 0 };
    int lowCutSlope{ 0 };

    // high cut parameters
    float highCutFreq{ 0 };
    int highCutSlope{ 0 };
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);


//==============================================================================
/**
*/
class EQtutAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    EQtutAudioProcessor();
    ~EQtutAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};

private:

    // float filter alias
    // filter types in IIR use 12 db/Oct cutoff for lowpass / highpass by default
    using Filter = juce::dsp::IIR::Filter<float>;

    // chained float filters
    // 1 - 4 filters, results in 12 - 48 db/Oct cutoff for lowpass / highpass
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    // declare 2 mono chains to represent full stereo signal
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    MonoChain leftChain, rightChain;
    
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQtutAudioProcessor)
};
