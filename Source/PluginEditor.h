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

    // --- DECLARE KNOBS ---

    Knob peakFreqKnob, peakGainKnob, peakQualityKnob;
    Knob lowCutFreqKnob, lowCutSlopeKnob;
    Knob highCutFreqKnob, highCutSlopeKnob;

    // --- CREATE ATTACHMENTS ---

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqKnobAtch, peakGainKnobAtch, peakQualityKnobAtch;
    Attachment lowCutFreqKnobAtch, lowCutSlopeKnobAtch;
    Attachment highCutFreqKnobAtch, highCutSlopeKnobAtch;



    std::vector<juce::Component*> getKnobs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQtutAudioProcessorEditor)
};
