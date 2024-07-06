/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    // draw knob background
    g.setColour(Colour(0xFF181818));
    g.fillEllipse(bounds);

    // draw knob border
    g.setColour(Colour(0xFF202020));
    g.drawEllipse(bounds, 2.f);

    if (auto* knob = dynamic_cast<Knob*>(&slider))
    {
        // draw knob position notch
        jassert(rotaryStartAngle < rotaryEngAngle);
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        auto center = bounds.getCentre();
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - knob->getTextHeight() * 1.5);

        Path p;
        p.addRoundedRectangle(r, 2.f);
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.setColour(Colour(0xFFFFFFFF));
        g.fillPath(p);

        // draw label
        g.setFont(knob->getTextHeight());
        auto text = knob->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, knob->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        g.setColour(Colour(0xFF181818));
        g.fillRect(r);

        g.setColour(Colour(0xFFFFFFFF));
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//==============================================================================

void Knob::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    // DEBUG: show knob bounding boxes
    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(
        g,
        sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng, endAng,
        *this
    );

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;


    // draw labels
    g.setColour(Colour(0xFFFFFFFF));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(1.f >= pos);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }

}

juce::Rectangle<int> Knob::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2); // two pixels below top of component

    return r;
}

juce::String Knob::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        return choiceParam->getCurrentChoiceName();
    }
    
    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        
        if (val >= 1000.f)
        {
            val /= 1000.f;
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse;
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
        { 
            str << "k";
        }
        str << suffix;
    }
    return str;
}

//==============================================================================

ResponseCurveComponent::ResponseCurveComponent(EQtutAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll(Colour(0xFF101010));

    auto responseArea = getLocalBounds();
    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;
    mags.resize(w);
    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();

    auto map = [outputMin, outputMax](double input)
        {
            return jmap(input, -24.0, 24.0, outputMin, outputMax);
        };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(Colour(0xFF181818));
    g.drawRoundedRectangle(responseArea.toFloat(), 1.0f, 4.f);
    g.setColour(Colour(0xFFFFFFFF));
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        // update the mono chain
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);

        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);

        // signal a repaint
        repaint();
    }
}

//==============================================================================
EQtutAudioProcessorEditor::EQtutAudioProcessorEditor(EQtutAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    
    // init labelled knobs
    peakFreqKnob    (*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainKnob    (*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualityKnob (*audioProcessor.apvts.getParameter("Peak Q"),    ""),

    lowCutFreqKnob  (*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    lowCutSlopeKnob (*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),

    highCutFreqKnob (*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    highCutSlopeKnob(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

    // init response curve
    responseCurveComponent  (audioProcessor),
    
    // init knob attachments
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
    
    peakFreqKnob.labels.add( {0.f, "20 Hz"} );
    peakFreqKnob.labels.add( {1.f, "20 kHz"} );
    peakGainKnob.labels.add({ 0.f, "-24 dB" });
    peakGainKnob.labels.add({ 1.f, "24 dB" });
    peakQualityKnob.labels.add({ 0.f, "0.1" });
    peakQualityKnob.labels.add({ 1.f, "10" });

    lowCutFreqKnob.labels.add({ 0.f, "20 Hz" });
    lowCutFreqKnob.labels.add({ 1.f, "20 kHz" });
    lowCutSlopeKnob.labels.add({ 0.f, "12 dB/Oct" });
    lowCutSlopeKnob.labels.add({ 1.f, "48 dB/Oct" });

    highCutFreqKnob.labels.add({ 0.f, "20 Hz" });
    highCutFreqKnob.labels.add({ 1.f, "20 kHz" });
    highCutSlopeKnob.labels.add({ 0.f, "12 dB/Oct" });
    highCutSlopeKnob.labels.add({ 1.f, "48 dB/Oct" });


    
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
void EQtutAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace juce;

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.fillAll(Colour(0xFF101010));
    
}

void EQtutAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    responseCurveComponent.setBounds(responseArea);

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
        &highCutSlopeKnob,

        &responseCurveComponent
    };
}
