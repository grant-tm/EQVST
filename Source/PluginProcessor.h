/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// contains parameters for peak, low cut, and high cut filters
struct ChainSettings
{
    // peak parameters
    float peakFreq{ 0 };
    float peakGainDB{ 0 };
    float peakQ{ 1.f };

    // low cut parameters
    float lowCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 };
    

    // high cut parameters
    float highCutFreq{ 0 };
    Slope highCutSlope{ Slope::Slope_12 };
};

enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

// float filter alias
   // filter types in IIR use 12 db/Oct cutoff for lowpass / highpass by default
using Filter = juce::dsp::IIR::Filter<float>;

// chained float filters
// 1 - 4 filters, results in 12 - 48 db/Oct cutoff for lowpass / highpass
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

// declare 2 mono chains to represent full stereo signal
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;


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

    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings& chainSettings);

    using Coefficients = Filter::CoefficientsPtr;
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    template<int Index, typename ChainType, typename CoefficientType>
    void updateFilter(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& filter, const CoefficientType& coefficients, const Slope& slope)
    {
        filter.template setBypassed<0>(true);
        filter.template setBypassed<1>(true);
        filter.template setBypassed<2>(true);
        filter.template setBypassed<3>(true);

        if (slope >= Slope_12) {
            updateFilter<0>(filter, coefficients);
        }
        if (slope >= Slope_24) {
            updateFilter<1>(filter, coefficients);
        }
        if (slope >= Slope_36) {
            updateFilter<2>(filter, coefficients);
        }
        if (slope >= Slope_48) {
            updateFilter<3>(filter, coefficients);
        }
    }

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    void updateFilters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQtutAudioProcessor)
};
