/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQtutAudioProcessorEditor::EQtutAudioProcessorEditor (EQtutAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    peakFreqKnobAtch    (audioProcessor.apvts, "Peak Freq",     peakFreqKnob),
    peakGainKnobAtch    (audioProcessor.apvts, "Peak Gain",     peakGainKnob),
    peakQualityKnobAtch (audioProcessor.apvts, "Peak Q",        peakQualityKnob),
    lowCutFreqKnobAtch  (audioProcessor.apvts, "LowCut Freq",   lowCutFreqKnob),
    lowCutSlopeKnobAtch (audioProcessor.apvts, "LowCut Slope",  lowCutSlopeKnob),
    highCutFreqKnobAtch (audioProcessor.apvts, "HighCut Freq",  highCutFreqKnob),
    highCutSlopeKnobAtch(audioProcessor.apvts, "HighCut Slope", highCutSlopeKnob)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for (auto* knob : getKnobs())
    {
        addAndMakeVisible(knob);
    }
    
    setSize (600, 400);
}

EQtutAudioProcessorEditor::~EQtutAudioProcessorEditor()
{
}

//==============================================================================
void EQtutAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void EQtutAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    lowCutFreqKnob.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeKnob.setBounds(lowCutArea);

    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    highCutFreqKnob.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeKnob.setBounds(highCutArea);
    
    peakFreqKnob.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainKnob.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualityKnob.setBounds(bounds);
}

std::vector<juce::Component*> EQtutAudioProcessorEditor::getKnobs()
{
    return
    {
        &peakFreqKnob,
        &peakGainKnob,
        &peakQualityKnob,
        
        &lowCutFreqKnob,
        &lowCutSlopeKnob,
        
        &highCutFreqKnob,
        &highCutSlopeKnob
    };
}
