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
class EQtutAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
public:
    EQtutAudioProcessorEditor (EQtutAudioProcessor&);
    ~EQtutAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { };

    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EQtutAudioProcessor& audioProcessor;

    juce::Atomic<bool> parametersChanged{ false };

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

    MonoChain monoChain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQtutAudioProcessorEditor)
};
