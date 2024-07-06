/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct Knob : juce::Slider
{
    Knob() : juce::Slider(
        juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox
    ){}
};

//==============================================================================
/**
*/
class EQtutAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EQtutAudioProcessorEditor (EQtutAudioProcessor&);
    ~EQtutAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EQtutAudioProcessor& audioProcessor;

    // peak knobs
    Knob peakFreqKnob, peakGainKnob, peakQualityKnob;
    
    // low cut knobs
    Knob lowCutFreqKnob, lowCutSlopeKnob;
    
    // high cut knobs
    Knob highCutFreqKnob, highCutSlopeKnob;

    std::vector<juce::Component*> getKnobs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQtutAudioProcessorEditor)
};
