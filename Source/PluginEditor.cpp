#include "PluginProcessor.h"
#include "PluginEditor.h"

//=============================================================================
// Knob Look And Feel
//=============================================================================

void LookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider
) {
    using namespace juce;

    auto bounds = Rectangle<float>(float(x), float(y), float(width), float(height));

    // draw knob background
    g.setColour(Colour(0xFFCCCCCC));
    g.fillEllipse(bounds);

    // draw knob border
    g.setColour(Colour(0xFF222222));
    g.drawEllipse(bounds, 2.f);

    if (auto* knob = dynamic_cast<Knob*>(&slider))
    {
        // draw knob position notch
        jassert(rotaryStartAngle < rotaryEngAngle);
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        auto center = bounds.getCentre();
        Rectangle<float> r;
        r.setLeft   (center.getX() - 2);
        r.setRight  (center.getX() + 2);
        r.setTop    (bounds.getY());
        r.setBottom((center.getY() - bounds.getY()) * 0.3f);

        Path p;
        p.addRoundedRectangle(r, 2.f);
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.setColour(Colour(0xFF222222));
        g.fillPath(p);

        // draw label
        g.setFont(float(knob->getTextHeight()));
        auto text = knob->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(float(strWidth + 4), float(knob->getTextHeight() + 2));
        r.setCentre(bounds.getCentre());
        g.setColour(Colour(0xFFCCCCCC));
        g.fillRect(r);

        g.setColour(Colour(0xFF222222));
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//=============================================================================
// Knob
//=============================================================================

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
        jmap(float(getValue()), float(range.getStart()), float(range.getEnd()), 0.f, 1.f),
        startAng, endAng,
        *this
    );

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;


    // draw labels
    g.setColour(Colour(0xFFFFFFFF));
    g.setFont(float(getTextHeight()));

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
        r.setSize(float(g.getCurrentFont().getStringWidth(str)), float(getTextHeight()));
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
        float val = float(getValue());
        
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

//=============================================================================
// Response Curve
//=============================================================================

ResponseCurveComponent::ResponseCurveComponent(EQtutAudioProcessor& p) :
    audioProcessor(p),
    leftPathProducer(audioProcessor.leftChannelFifo),
    rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    updateChain();

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

    g.fillAll(Colour(0xFF111111));

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea();
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

    responseCurve.startNewSubPath(float(responseArea.getX()), float(map(mags.front())));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(float(responseArea.getX() + i), float(map(mags[i])));
    }

    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(float(responseArea.getX()), float(responseArea.getY())));
    g.setColour(Colours::red);
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.5f));

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(float(responseArea.getX()), float(responseArea.getY())));
    g.setColour(Colours::green);
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.5f));
    
    g.setColour(Colour(0xFF222222));
    g.drawRoundedRectangle(getRenderArea().toFloat(), 1.0f, 4.f);
    g.setColour(Colour(0xFFCCCCCC));
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    // -- DRAW FREQUENCY GRIDLINES --

    Array<float> gridXLines { 
        20, 30, 40, 50, 60, 70, 80, 90,
        100, 200, 300, 400, 500, 600, 700, 800, 900,
        1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000,
        10000, 12000, 14000, 16000, 18000, 20000 
    };

    Array<float> xs;
    for (auto x : gridXLines)
    {
        auto normX = mapFromLog10(x, 20.f, 20000.f);
        xs.add(left + width * normX);
    }

    g.setColour(Colour(0xFFAAAAAA));
    for (auto x : xs)
    {
        g.drawVerticalLine(int(x), float(top), float(bottom));
    }

    // -- DRAW GAIN GRIDLINES --

    Array<float> gridYLines { -24, -12, 0, 12, 24 };
    for (auto y : gridYLines)
    {
        auto mapY = jmap(y, -24.f, 24.f, float(bottom), float(top));
        g.setColour(y == 0.f ? Colour(0xFF00CC00) : Colour(0xFF222222));
        g.drawHorizontalLine(int(mapY), float(left), float(right));
    }

    // -- DRAW FREQUENCY LABELS --

    g.setColour(Colour(0xFFCCCCCC));
    const int fontHeight = 10;
    g.setFont(fontHeight);

    for (int i = 0; i < gridXLines.size(); ++i)
    {
        auto f = gridXLines[i];
        auto x = xs[i];

        if (f != 50.f && f != 100.f && f != 500.f && f != 1000.f && f != 5000.f && f != 10000.f)
        {
            continue;
        }

        String str;
        str << f;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(int(x), 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    // -- DRAW GAIN LABELS --

    for (auto y : gridYLines)
    {
        auto mapY = jmap(y, -24.f, 24.f, float(bottom), float(top));

        String str;
        if (y > 0)
            str << "+";
        str << y;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(int(r.getCentreX()), int(mapY));

        g.setColour(mapY == 0.f ? Colour(0xFF00CC00) : Colour(0xFF222222));
        g.drawFittedText(str, r, juce::Justification::centred, 1);

        str.clear();
        str << (y - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colour(0xFFCCCCCC));
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);

    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            // shift mono buffer
            auto size = tempIncomingBuffer.getNumSamples();
            juce::FloatVectorOperations::copy(
                monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size),
                monoBuffer.getNumSamples() - size
            );

            // copy temp buffer to end of mono buffer
            juce::FloatVectorOperations::copy(
                monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                tempIncomingBuffer.getReadPointer(0, 0),
                size
            );

            // send mono buffer to FFT data generator
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / (double(fftSize));

    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, float(binWidth), -48.f);
        }
    }

    while (pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void ResponseCurveComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);

    if (parametersChanged.compareAndSetBool(false, true))
    {
        updateChain();
    }

    repaint();
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//=============================================================================
// Editor
//=============================================================================
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
    peakGainKnob.labels.add({ 1.f, "+24 dB" });
    peakQualityKnob.labels.add({ 0.f, "0.1" });
    peakQualityKnob.labels.add({ 1.f, "10.0" });

    lowCutFreqKnob.labels.add({ 0.f, "20 Hz" });
    lowCutFreqKnob.labels.add({ 1.f, "20 kHz" });
    lowCutSlopeKnob.labels.add({ 0.f, "12" });
    lowCutSlopeKnob.labels.add({ 1.f, "48" });

    highCutFreqKnob.labels.add({ 0.f, "20 Hz" });
    highCutFreqKnob.labels.add({ 1.f, "20 kHz" });
    highCutSlopeKnob.labels.add({ 0.f, "12" });
    highCutSlopeKnob.labels.add({ 1.f, "48" });


    
    for (auto* knob : getKnobs())
    {
        addAndMakeVisible(knob);
    }

    setSize (600, 488);
}

EQtutAudioProcessorEditor::~EQtutAudioProcessorEditor()
{

}

void EQtutAudioProcessorEditor::paint(juce::Graphics& g)
{
    using namespace juce;
    g.fillAll(Colour(0xFF111111));
    
}

void EQtutAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(int(bounds.getHeight() * 0.33f));
    responseCurveComponent.setBounds(responseArea);

    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(int(bounds.getWidth() * 0.33f));
    lowCutFreqKnob.setBounds(lowCutArea.removeFromTop(int(lowCutArea.getHeight() * 0.5f)));
    lowCutSlopeKnob.setBounds(lowCutArea);

    auto highCutArea = bounds.removeFromRight(int(bounds.getWidth() * 0.5f));
    highCutFreqKnob.setBounds(highCutArea.removeFromTop(int(highCutArea.getHeight() * 0.5f)));
    highCutSlopeKnob.setBounds(highCutArea);
    
    peakFreqKnob.setBounds(bounds.removeFromTop(int(bounds.getHeight() * 0.33f)));
    peakGainKnob.setBounds(bounds.removeFromTop(int(bounds.getHeight() * 0.5f)));
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
