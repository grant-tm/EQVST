/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override { }
};

struct Knob : juce::Slider
{
    Knob(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) 
        : juce::Slider(
            juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox
        ),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~Knob()
    {
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override { };
    
    juce::Rectangle<int> getSliderBounds() const;
    
    int getTextHeight() const { return 14; }

    juce::String getDisplayString() const;


private:
    LookAndFeel lnf;
    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
public:
    ResponseCurveComponent(EQtutAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { };
   
    void timerCallback() override;

    void paint(juce::Graphics& g) override;

private:
    EQtutAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };
    MonoChain monoChain;
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

    // --- CREATE RESPONSE CURVE ---
    ResponseCurveComponent responseCurveComponent;

    // --- CREATE ATTACHMENTS ---
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqKnobAtch, peakGainKnobAtch, peakQualityKnobAtch;
    Attachment lowCutFreqKnobAtch, lowCutSlopeKnobAtch;
    Attachment highCutFreqKnobAtch, highCutSlopeKnobAtch;

    std::vector<juce::Component*> getKnobs();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQtutAudioProcessorEditor)
};
