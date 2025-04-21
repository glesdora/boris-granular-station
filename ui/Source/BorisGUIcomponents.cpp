#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>
#include "MoonJKnob.h"

enum ValueLabelPos
{
    NoLabel,
    Bottom,
    Front
};

enum TextPosition
{
    Top,
    Left
};

class BorisGUIComp
{
public:
    BorisGUIComp(float preferredratio) : _preferredratio(preferredratio) {}
    float getPreferredRatio() { return _preferredratio; }
    virtual TextPosition placeText() { return Top; }
    virtual ValueLabelPos showValLab() { return Bottom; }
    virtual double getValue() = 0;
	virtual double valueToDisplay() { return getValue(); }

private:
    const float _preferredratio;
};

class ValueLabel2 : public Component, public Value::Listener
{
public:
	ValueLabel2(BorisGUIComp* c, Value& v, int dp, const String& uom) : value(v), dp(dp), uom(uom)
    {
        component = c;
        if (Slider* slider = dynamic_cast<Slider*>(component); slider != nullptr) {
            this->slider = slider;
        }
		value.addListener(this);

		String stringvalue = value.toString();
        if (uom == " dB") {
			stringvalue = String(Decibels::gainToDecibels(slider->getValue()));
        }
        text = stringvalue + uom;
    }

    void valueChanged(juce::Value&) override {
        text = "";
        if (prepX) { text = "x "; }
        
        String stringvalue = value.toString();
        if (uom == " dB") {
            stringvalue = String(Decibels::gainToDecibels(slider->getValue()));
        }
        text = stringvalue + uom;

        repaint();
    }

    void paint(Graphics& g) override
    {
        g.setFont(font);
        g.setColour(borisPalette[inactive]);
        g.drawText(text, getLocalBounds(), Justification::centred);
    }

    void setUnityOfMeasure(const String& unity) { this->uom = unity; }
    void setDecimalPlaces(int dp) { this->dp = dp; }
    void prependX() { prepX = true; }

    void setTransmitEvents(bool b)
    {
        if (slider != nullptr) {
            forwardevents = b;
        }
    }

    void mouseDown(const juce::MouseEvent& event) override {
        if (forwardevents) {
            slider->mouseDown(event.getEventRelativeTo(slider));
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        if (forwardevents) {
            slider->mouseDrag(event.getEventRelativeTo(slider));
        }
    }

    void mouseUp(const juce::MouseEvent& event) override {
        if (forwardevents) {
            slider->mouseUp(event.getEventRelativeTo(slider));
        }
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override {
        if (forwardevents) {
            slider->mouseDoubleClick(event.getEventRelativeTo(slider));
        }
    }

private:
    Value value;
    String text;
    Font font{ "Carlito Bold", STEXTSIZE, 0 };
    bool forwardevents = false;
    BorisGUIComp* component;
    Slider* slider;
    String uom = " ";
    int dp = 2;
    bool prepX = false;
};

class ValueLabel : public Component
{
public:
    ValueLabel(BorisGUIComp* c)
    {
        component = c;
        if (Slider *slider = dynamic_cast<Slider*>(component); slider != nullptr) {
            this->slider = slider;
        }
    }

    void paint(Graphics& g) override
    {
        text = "";
        if (prepX) { text = "x "; }
        //text += String(component->getValue(), dp) + uom;
		text += String(component->valueToDisplay(), dp) + uom;

        g.setFont(font);
        g.setColour(borisPalette[inactive]);
        g.drawText(text, getLocalBounds(), Justification::centred);
    }

    void setUnityOfMeasure(String unity) { this->uom = unity; }
	void setDecimalPlaces(int dp) { this->dp = dp; }
    void prependX() { prepX = true; }

    void setTransmitEvents(bool b)
    {
        if (slider != nullptr) {
            forwardevents = b;
        }
    }

    void mouseDown(const juce::MouseEvent& event) override {
        if (forwardevents) {
            slider->mouseDown(event.getEventRelativeTo(slider));
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        if (forwardevents) {
            slider->mouseDrag(event.getEventRelativeTo(slider));
        }
    }

    void mouseUp(const juce::MouseEvent& event) override {
        if (forwardevents) {
            slider->mouseUp(event.getEventRelativeTo(slider));
        }
    }

	void mouseDoubleClick(const juce::MouseEvent& event) override {
		if (forwardevents) {
			slider->mouseDoubleClick(event.getEventRelativeTo(slider));
		}
	}

private:
    String text;
    Font font {"Carlito Bold", STEXTSIZE, 0};
    bool forwardevents = false;
    BorisGUIComp* component;
    Slider* slider;
    String uom;
    int dp;
    bool prepX = false;
};

class BorisToggle : public juce::ToggleButton, public BorisGUIComp
{
public:
    BorisToggle (const String& buttonName)
        : ToggleButton (buttonName), BorisGUIComp(1)
    {
        //onClick = [this]() { value.setValue(getToggleState()); };
    }

    //juce::Value& getValueObject() { return value; }

    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds();
        auto w = bounds.getWidth();

        Rectangle<float> buttonArea = bounds.toFloat();
        Rectangle<float> r = buttonArea.reduced(2.0f);

        g.setColour(borisPalette[border]);
        Path p;
        p.addEllipse (r);
        g.strokePath (p, PathStrokeType (2));

        if (icon)
        {
            if (getToggleState()) {
                icon->replaceColour(offcolour, oncolour);
            } else {
                icon->replaceColour(oncolour, offcolour);
            }
            icon->drawWithin (g, r.reduced(r.getWidth()*0.0f), juce::RectanglePlacement::centred, 1.0f);
        }
    }

    ValueLabelPos showValLab() override { return NoLabel; }

    double getValue() override { return getToggleState() ? 1.0 : 0.0; }

    void loadIcon(File* iconFile)
    {
        std::unique_ptr<juce::XmlElement> iconXML = juce::XmlDocument::parse(*iconFile);

        if (iconXML)
        {
            icon = juce::Drawable::createFromSVG(*iconXML);
            icon->replaceColour(Colours::white, borisPalette[button]);
            icon->replaceColour(Colours::black, oncolour);
        }
    }

    void setColours(Colour on, Colour off)
    {
        oncolour = on;
        offcolour = off;
    }

private:
    std::unique_ptr<juce::Drawable> icon;
    Colour oncolour = borisPalette[active];
    Colour offcolour = borisPalette[inactive];
    //Value value;
};

class BorisSlider : public juce::Slider, public BorisGUIComp
{
protected:
    BorisSlider(const String& componentName, float preferredratio) : Slider(componentName), BorisGUIComp(preferredratio) {}

public:
    double snapValue(double value, DragMode) override
    {
        double minValue = getMinimum();
        double maxValue = getMaximum();

        // Convert value to normalized 0-1 position
        double normalizedPos = std::pow((value - minValue) / (maxValue - minValue), skewFactor);

        // Calculate the step size in the "visual space"
        double stepSize = 1.0 / (numSteps - 1);

        // Snap the position to the nearest step
        double snappedPos = std::round(normalizedPos / stepSize) * stepSize;

        // Convert back from position to actual value
        return minValue + (maxValue - minValue) * std::pow(snappedPos, 1.0 / skewFactor);
    }

    void setSkewAndSteps(double skew, int steps)
    {
        setSkewFactor(skew);

        skewFactor = skew;
        numSteps = steps;
    }

private:
    double skewFactor = 1.0;
    int numSteps = 101;
};

class BorisVerticalSlider : public juce::Slider, public BorisGUIComp
{
public:
    BorisVerticalSlider(const String& componentName) : Slider(componentName), BorisGUIComp(0.125f)
    {
        setSliderStyle(SliderStyle::LinearBarVertical);
        setTextBoxStyle(NoTextBox, true, 0, 0);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto sliderPos = bounds.getHeight() - getPositionOfValue(getValue());
        auto cornersize = bounds.getWidth()*0.25f;

        Path roundedRect;
        roundedRect.addRoundedRectangle(bounds, cornersize);
        
        g.setColour(borisPalette[inactive]);        // Or replace with colour1
        g.fillPath(roundedRect);

        Path activePath;
        activePath.addRoundedRectangle(bounds.removeFromBottom(sliderPos), cornersize*0.5f);

        g.setColour(borisPalette[active]);
        g.saveState();                 
        g.reduceClipRegion(activePath);
        g.fillPath(roundedRect);
        g.restoreState();
    }

    double getValue() override { return Slider::getValue(); }
};

class BorisVolumeSlider : public BorisVerticalSlider
{
public:
	BorisVolumeSlider(const String& componentName) : BorisVerticalSlider(componentName) {}
};

class BorisHorizontalSlider : public juce::Slider, public BorisGUIComp
{
public:
    BorisHorizontalSlider(const String& componentName) : Slider(componentName), BorisGUIComp(8)
    {
        setSliderStyle(SliderStyle::LinearBar);
        setTextBoxStyle(NoTextBox, true, 0, 0);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto sliderPos =  getPositionOfValue(getValue());
        auto cornersize = bounds.getHeight()*0.25f;

        Path roundedRect;
        roundedRect.addRoundedRectangle(bounds, cornersize);
        
        g.setColour(borisPalette[inactive]);
        g.fillPath(roundedRect);

        Path activePath;
        activePath.addRoundedRectangle(bounds.removeFromLeft(sliderPos), cornersize*0.5f);

        g.setColour(borisPalette[active]);
        g.saveState();
        g.reduceClipRegion(activePath);
        g.fillPath(roundedRect);
        g.restoreState();
    }

    double getValue() override { return Slider::getValue(); }
};

class BorisNumberBoxSlider : public BorisSlider
{
public:
    BorisNumberBoxSlider(const String& componentName) : BorisSlider(componentName, 4)
    {
        setSliderStyle(SliderStyle::LinearVertical);
        setTextBoxStyle(TextBoxBelow, false, 0, 0);
        setSliderSnapsToMousePosition(false);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto sliderPos = bounds.getHeight() - getPositionOfValue(getValue());
        auto cornersize = bounds.getWidth()*0.05f;

        Path roundedRect;
        roundedRect.addRoundedRectangle(bounds, cornersize);
        
        g.setColour(borisPalette[back]);
        g.fillPath(roundedRect);

        // Path activePath;
        // activePath.addRoundedRectangle(bounds.removeFromLeft(sliderPos), cornersize*0.5f);

        // g.setColour(borisPalette[active]);
        // g.saveState();
        // g.reduceClipRegion(activePath);
        // g.fillPath(roundedRect);
        // g.restoreState();

        g.setColour(borisPalette[border]);
        g.strokePath(roundedRect, PathStrokeType(1.0f));

        // g.setColour(borisPalette[led]);
        // g.drawText(String(getValue(), 2), bounds, Justification::centred);
    }

    ValueLabelPos showValLab() override { return Front; }

    double getValue() override { return Slider::getValue(); }
};

class BorisLeftNumBoxSlider : public BorisNumberBoxSlider
{
public:
    BorisLeftNumBoxSlider(const String& componentName) : BorisNumberBoxSlider(componentName)
    {
        setTextBoxStyle(TextBoxLeft, false, 0, 0);
    }

    TextPosition placeText() override { return Left; }
};

class BorisDial : /*public juce::Slider, public BorisGUIComp*/ public BorisSlider
{
public:
	BorisDial(const String& componentName) : BorisSlider(componentName, 1.3f)
    {
        setSliderStyle(SliderStyle::RotaryHorizontalVerticalDrag);
        setTextBoxStyle(NoTextBox, true, 0, 0);

        led_ptrs.resize(numberOfLeds);
        for (int i = 0; i < numberOfLeds; i++) {
            led_ptrs[i].reset(new MoonJLed(false));
            addAndMakeVisible(led_ptrs[i].get());
        }
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto center = bounds.getCentre();
        center.addXY(0, leds_to_in_rad * 0.5f);

        auto sliderPos =  static_cast<float>(valueToProportionOfLength(getValue()));
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // g.setColour(Colours::orange);
        // g.drawRect(bounds, 1.0f);

        g.setColour(borisPalette[back]);
        g.fillEllipse(center.x - innerBorderRadius, center.y - innerBorderRadius, innerBorderRadius * 2, innerBorderRadius * 2);

        g.setColour(borisPalette[border]);
        g.strokePath(borderBand, PathStrokeType(innerBorderWidth));
        
        g.setColour(borisPalette[back].darker());
        g.strokePath(outline, PathStrokeType(outerBorderWidth));

        g.setColour(borisPalette[border]);
        g.strokePath(emptyCursorBand, PathStrokeType(1.0f));

        Point<float> indicatorPos(
            center.x + std::sin(toAngle) * cursorArcRadius,
            center.y + std::cos(toAngle) * cursorArcRadius * (-1.0f)
        );

        float zoom = 1.0f;

        Rectangle<float> arrowBounds(-arrowheight, -arrowheight, arrowheight * 2.0f, arrowheight * 2.0f);
        Image arrowImage = Image(Image::ARGB, arrowheight * zoom * 2.0f, arrowheight * zoom * 2.0f, true);
        Graphics arrowGraphics(arrowImage);
        AffineTransform transform = AffineTransform::rotation(toAngle).translated(arrowheight * zoom, arrowheight * zoom);

        arrowGraphics.setColour(borisPalette[led]);
        arrowGraphics.fillPath(arrowPath, transform);

        // glow effect
        float glowradius = 0.1f * arrowheight * zoom;
        GlowEffect glow;
        glow.setGlowProperties(glowradius, borisPalette[led]);
        Image glowedImage = arrowImage.createCopy();
        Graphics glowGraphics(glowedImage);

        glow.applyEffect(arrowImage, glowGraphics, 1.0f, 1.0f);
        g.drawImage(glowedImage, arrowBounds.translated(indicatorPos.x, indicatorPos.y), RectanglePlacement::stretchToFit);

        if (led_ptrs.size() == numberOfLeds) {
            int pointedLed = round(sliderPos * (numberOfLeds-1));
            for (int i = 0; i <= pointedLed; i++) {
                led_ptrs[i]->setBrightness((i+1)/((float)(pointedLed+1)));
            }
            for (int i = pointedLed + 1; i < numberOfLeds; i++) {
                led_ptrs[i]->setBrightness(0.0f);
            }
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        float boundsratio = static_cast<float>(bounds.getWidth()) / static_cast<float>(bounds.getHeight());
        if (boundsratio > getPreferredRatio()) {
            bounds.setWidth(bounds.getHeight() * getPreferredRatio());
        } else {
            bounds.setHeight(bounds.getWidth() / getPreferredRatio());
        }

        ledsArcRadius = bounds.getWidth() * 0.45f;

        float outertoledratio = 0.86f;
        float bandcircleratio = 1.0f - (2 /getPreferredRatio() - 1.0f) / outertoledratio;

        outerBorderRadius = ledsArcRadius * outertoledratio;
        cursorBandWidth = outerBorderRadius * bandcircleratio;        // Change this to change ratio between cursor band and inner circle

        ledSize = ledsArcRadius * 0.1f;       // 0.088f
        innerBorderRadius = outerBorderRadius - cursorBandWidth;
        cursorArcRadius = innerBorderRadius + cursorBandWidth * 0.5f;

        leds_to_in_rad = ledsArcRadius - innerBorderRadius;

        outerBorderWidth = 0.5f;
        innerBorderWidth = outerBorderRadius * 0.026f;

        arrowheight = cursorBandWidth * 0.4f;
        ledswidth = ledSize * 0.5f;

        borderBand.clear();
        outline.clear();
        ledsBand.clear();
        arrowPath.clear();

        auto center = bounds.getCentre();
        center.addXY(0, leds_to_in_rad * 0.5f);

        borderBand.addCentredArc(center.x, center.y, innerBorderRadius, innerBorderRadius, 0.0f, 0.0f, 2*float_Pi, true);
        outline.addCentredArc(center.x, center.y, outerBorderRadius, outerBorderRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        ledsBand.addCentredArc(center.x, center.y, ledsArcRadius, ledsArcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        arrowPath.addTriangle(-arrowheight * 0.5f, arrowheight * 0.5f, arrowheight * 0.5f, arrowheight * 0.5f, 0.0f, -arrowheight * 0.5f);

        float emptyStartAngle = rotaryStartAngle - 1.5f * arrowheight / outerBorderRadius;
        float emptyEndAngle = rotaryEndAngle + 1.5f * arrowheight / outerBorderRadius;

        emptyCursorBand.clear();
        emptyCursorBand.addCentredArc(center.x, center.y, cursorArcRadius + cursorBandWidth * 0.5f, cursorArcRadius + cursorBandWidth * 0.5f, 0.0f, emptyStartAngle, emptyEndAngle, true);
        // emptyCursorBand.lineTo(center.x + std::sin(emptyEndAngle) * (cursorArcRadius - cursorBandWidth * 0.5f), center.y + std::cos(emptyEndAngle) * (cursorArcRadius - cursorBandWidth * 0.5f) * (-1.0f));
        emptyCursorBand.addCentredArc(center.x, center.y, cursorArcRadius - cursorBandWidth * 0.5f, cursorArcRadius - cursorBandWidth * 0.5f, 0.0f, emptyEndAngle, emptyStartAngle, false);
        // emptyCursorBand.lineTo(center.x + std::sin(emptyStartAngle) * (cursorArcRadius + cursorBandWidth * 0.5f), center.y + std::cos(emptyStartAngle) * (cursorArcRadius + cursorBandWidth * 0.5f) * (-1.0f));

        if (led_ptrs.size() == numberOfLeds) {
            for (int i = 0; i < numberOfLeds; i++) {
                float angle = rotaryStartAngle + i * (rotaryEndAngle - rotaryStartAngle) / (numberOfLeds - 1);

                Point<float> ledPos(
                    center.x + std::sin(angle) * (ledsArcRadius),
                    center.y + std::cos(angle) * (ledsArcRadius) * (-1.0f)
                );

                ledTransform = AffineTransform::rotation(angle).translated(ledPos.x, ledPos.y);

                Rectangle<int> ledBounds;
                ledBounds = Rectangle<int>(-ledSize * 0.5f, -ledSize * 0.5f, ledSize, ledSize);

                led_ptrs[i]->setBounds(ledBounds);
                led_ptrs[i]->setTransform(ledTransform);
            }
        }
    }

    double getValue() override { return Slider::getValue(); }

private:
    const int numberOfLeds = 9;

    float rotaryStartAngle = -2.0f;
    float rotaryEndAngle = 2.0f;

    float ledsArcRadius;
    float outerBorderRadius;
    float cursorArcRadius;
    float innerBorderRadius;

    float cursorBandWidth;
    float outerBorderWidth;
    float innerBorderWidth;

    float ledSize;
    float ledswidth;
    float arrowheight;

    float leds_to_in_rad;

    Path borderBand;
    Path arrowBand;
    Path outline;
    Path ledsBand;
    Path emptyCursorBand;
    Path arrowPath;

    AffineTransform ledTransform;
    std::vector<std::unique_ptr<MoonJLed>> led_ptrs;
};

class BorisDenDial : public BorisDial
{
public:
    BorisDenDial(const String& componentName) : BorisDial(componentName) {}

    ValueLabelPos showValLab() override { return NoLabel; }
};

class BorisFdbDial : public BorisDial
{
public:
    BorisFdbDial(const String& componentName) : BorisDial(componentName) {}

    ValueLabelPos showValLab() override { return Front; }
};

class BorisPtcDial : public BorisDial
{
public:
    BorisPtcDial(const String& componentName) : BorisDial(componentName)
    {
    }

    double snapValue(double value, DragMode) override
    {
        double semitoneRatio = std::pow(2.0, 1.0 / 48.0);

        // Determine how many semitone steps the value is away from the base
        double n = std::round(std::log(value) / std::log(semitoneRatio));

        // Recalculate the exact snapped value
        return std::pow(semitoneRatio, n);
    }

    double valueToProportionOfLength(double value) override
    {
        double minValue = getMinimum();
        double maxValue = getMaximum();
        return (std::log(value / minValue)) / (std::log(maxValue / minValue));
    }

    double proportionOfLengthToValue(double proportion) override
    {
        double minValue = getMinimum();
        double maxValue = getMaximum();
        return minValue * std::pow(maxValue / minValue, proportion);
    }
};

class BorisLenDial : public BorisDial
{
public:
	BorisLenDial(const String& componentName) : BorisDial(componentName)
	{
		setSkewAndSteps(0.4199, 101);
	}
};

class BorisMoonJKnobWrapper : public MoonJKnob, public BorisGUIComp
{
public:
    BorisMoonJKnobWrapper(const String& componentName) : MoonJKnob(componentName), BorisGUIComp(1)
    {
    }

    double getValue() override { return Slider::getValue(); }

    ValueLabelPos showValLab() override { return NoLabel; }
};

class BorisInvisibleSlider : public juce::Slider, public BorisGUIComp
{
public:
    BorisInvisibleSlider(const String& componentName) : Slider(componentName), BorisGUIComp(0)
    {
    }

    ValueLabelPos showValLab() override { return NoLabel; }

    double getValue() override { return Slider::getValue(); }
};

class BorisSubPanel : public Component
{
public:
    enum SubPanelShape {
        Rounded,
        Plain
    };

    BorisSubPanel(Colour colour, SubPanelShape shape = Rounded) : colour(colour), subshape(shape) {
    }

    void paint(Graphics& g) override {
        auto bounds = getLocalBounds();

        g.setColour(colour);
        if (subshape == Rounded) {     
            g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
        } else {
            g.fillRect(bounds.toFloat());
        }
    }

protected:
    Colour colour;
    SubPanelShape subshape;
};

class BorisSplitSubPanel : public BorisSubPanel
{
public:
    BorisSplitSubPanel(Colour colour, SubPanelShape shape = Rounded,
                       float splitratio = 0.5f, bool splitHorizontally = false)
                       : BorisSubPanel(colour, shape), splitratio(splitratio),
                         splitHorizontally(splitHorizontally)
    {
    }

    void paint(Graphics& g) override {
        auto bounds = getLocalBounds();
        
        g.setColour(colour.withAlpha(alpha1));
        g.saveState();
        g.reduceClipRegion(firstPath);
        if (subshape == Rounded)
            g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
        else
            g.fillRect(bounds);
        g.restoreState();

        g.setColour(colour.withAlpha(alpha2));
        g.saveState();
        g.reduceClipRegion(secondPath);
        if (subshape == Rounded)
            g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
        else
            g.fillRect(bounds);
        g.restoreState();
    }

    void resized() {
        auto bounds = getLocalBounds();
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();

        firstPath.clear();
        secondPath.clear();

        if (!splitHorizontally) {
            firstPath.addRectangle(bounds.withTrimmedRight(w * (1.0f - splitratio)));
            secondPath.addRectangle(bounds.withTrimmedLeft(w * splitratio));
        } else {
            firstPath.addRectangle(bounds.withTrimmedBottom(h * (1.0f - splitratio)));
            secondPath.addRectangle(bounds.withTrimmedTop(h * splitratio));
        }
    }

private:
    Path firstPath;
    Path secondPath;

    float alpha1 = 0.3f;
    float alpha2 = 0.5f;

    float splitratio;
    bool splitHorizontally;
};

enum CompPanelStyle
{
    Invisible,
    TextSliderValue,
    TextSlider,
    TextNumBox,
    LeftTextNumBox,
    RoundToggle
};

class ComponentPanel : public Component
{
public:
    ComponentPanel(Component* c, const char* text, int textsize, int lines, String unityofmeasure, int decimalplaces, CompPanelStyle style)
                    : component(c), text(text), textsize(textsize), lines(lines), unityofmeasure(unityofmeasure), decimalplaces(decimalplaces), style(style)
    {
        name = String(String::fromUTF8(text));
        addAndMakeVisible(component);
        //valueLabel.reset(new ValueLabel(dynamic_cast<BorisGUIComp*>(component)));

		if (auto slider = dynamic_cast<Slider*>(component); slider != nullptr) {
            valueLabel.reset(new ValueLabel2(dynamic_cast<BorisGUIComp*>(component), slider->getValueObject(), decimalplaces, unityofmeasure));
            addAndMakeVisible(*valueLabel.get());
		}
        /*else if (auto toggle = dynamic_cast<BorisToggle*>(component); toggle != nullptr) {
            valueLabel.reset(new ValueLabel2(dynamic_cast<BorisGUIComp*>(component), toggle->getValueObject(), decimalplaces, unityofmeasure));
            addAndMakeVisible(*valueLabel.get());
        }*/
    }

    void paint(Graphics& g) {           //maybe control here if not InvisibleSlider?
        // g.setColour(Colours::orange);
        // g.drawRect(getLocalBounds(), 1);

        // g.setColour(Colours::yellow);
        // g.drawRect(text_area, 1);
        // g.setColour(Colours::blue);
        // g.drawRect(value_area, 1);

        // g.setColour(Colours::green);
        // g.drawRect(component->getBounds(), 1);

        g.setFont(font);
        g.setFont(textsize);
        g.setColour(borisPalette[inactive]);

        if (style == LeftTextNumBox) {
            g.drawFittedText(name, text_area, Justification::centredRight, 2, 1.0f);
        } else {
            g.drawFittedText(name, text_area, Justification::centred, 2, 1.0f);
        }
    }

    void resized() {
        auto bounds = getLocalBounds();

        if (BorisGUIComp* c = dynamic_cast<BorisGUIComp*>(component); c != nullptr) {
            auto valLabPos = c->showValLab();

            if (style == TextNumBox) {
                Rectangle<int> textband;
                Rectangle<int> compbounds;

                int cushion = 2;            // pixels between text and component

                float prefratio = c->getPreferredRatio();
                float comph = compw / prefratio;

                if (bounds.getHeight() >= comph && bounds.getWidth() >= compw) {
                    compbounds = bounds.removeFromBottom(comph);
                    compbounds = compbounds.withSizeKeepingCentre(compw, comph);
                    bounds.removeFromBottom(cushion);
                    textband = bounds.removeFromBottom(textsize * lines);

                    comp_area = compbounds;
                    text_area = textband;

                    valueLabel->setUnityOfMeasure(unityofmeasure);
                    valueLabel->setTransmitEvents(true);
                    if (auto p = dynamic_cast<BorisPtcDial*>(component); p != nullptr) {
                        valueLabel->prependX();
                    }

                    component->setBounds(comp_area);
                    valueLabel->setBounds(comp_area);
                } 
                  else {
                }
            } else if (style == LeftTextNumBox) {
                Rectangle<int> textband;
                Rectangle<int> compbounds;

                int cushion = 4;            // pixels between text and component

                float prefratio = c->getPreferredRatio();
                float comph = compw / prefratio;

                if (bounds.getHeight() >= comph && bounds.getWidth() >= compw) {
                    compbounds = bounds.removeFromRight(compw);
                    compbounds = compbounds.withSizeKeepingCentre(compw, comph);
                    bounds.removeFromRight(cushion);
                    textband = bounds;

                    comp_area = compbounds;
                    text_area = textband;

                    valueLabel->setUnityOfMeasure(unityofmeasure);
                    valueLabel->setTransmitEvents(true);
                    if (auto p = dynamic_cast<BorisPtcDial*>(component); p != nullptr) {
                        valueLabel->prependX();
                    }

                    component->setBounds(comp_area);
                    valueLabel->setBounds(comp_area);
                }

            } else if (style == RoundToggle) {
                Rectangle<int> compbounds;

                float prefratio = c->getPreferredRatio();
                float comph = compw / prefratio;

                if (bounds.getHeight() >= comph && bounds.getWidth() >= compw) {
                    compbounds = bounds.withSizeKeepingCentre(compw, comph);
                    comp_area = compbounds;

                    component->setBounds(comp_area);
                }
            }
            else {
                Rectangle<int> bottomband;
                if (valLabPos == Bottom) {
                    bottomband = bounds.removeFromBottom(STEXTSIZE * 1.2f);
                }

                Rectangle<int> textband;
                if (c->placeText() == Top) {
                    auto bheight = textsize * 1.2f * lines;
                    textband = bounds.removeFromTop(bheight);
                } else if (c->placeText() == Left) {
                    textband = bounds.removeFromLeft(textsize * 2.0f);
                }


                Rectangle<int> comp_bounds = bounds;
                float panelratio = static_cast<float>(comp_bounds.getWidth()) / static_cast<float>(comp_bounds.getHeight());
                float prefratio = c->getPreferredRatio();

                if (prefratio) {
                    auto bot = comp_bounds.getBottom();

                    if (panelratio < prefratio) {
                        comp_bounds = comp_bounds.withSizeKeepingCentre(comp_bounds.getWidth(), comp_bounds.getWidth() / prefratio);
                    } else {
                        comp_bounds = comp_bounds.withSizeKeepingCentre(comp_bounds.getHeight() * prefratio, comp_bounds.getHeight());
                    }

                    if (compw < comp_bounds.getWidth()) {
                        float ratio = static_cast<float>(comp_bounds.getWidth()) / static_cast<float>(comp_bounds.getHeight());
                        comp_bounds = comp_bounds.withSizeKeepingCentre(compw, compw / ratio);
                    }

                    if (valLabPos == Front) {
                        comp_bounds.translate(0, (bot - comp_bounds.getBottom()));
                    }
                }

                comp_area = comp_bounds;
                component->setBounds(comp_bounds);

                Rectangle<int> upper_margin(bounds.getTopLeft(), comp_bounds.getTopLeft());
                Rectangle<int> lower_margin(comp_bounds.getBottomRight(), bounds.getBottomRight());

                // text_area = upper_margin.getUnion(topBand);
                // value_area = lower_margin.getUnion(bottomBand);

                text_area = textband;
                value_area = bottomband;

                valueLabel->setUnityOfMeasure(unityofmeasure);
				valueLabel->setDecimalPlaces(decimalplaces);

                if (auto p = dynamic_cast<BorisPtcDial*>(component); p != nullptr) {
                    valueLabel->prependX();
                }

                if (c->showValLab() == Bottom) {
                    valueLabel->setBounds(value_area);
                } else if (c->showValLab() == Front) {
                    valueLabel->setBounds(comp_area);
                    valueLabel->setTransmitEvents(true);
                }
            }
        }
    }

    void setBounds(Rectangle<int> bounds, int compw) {
        this->compw = compw;

        Component::setBounds(bounds);
    }

    void setBounds(Rectangle<int> bounds) {
        compw = bounds.getWidth();
        Component::setBounds(bounds);
    }

private:
    Component* component;
    Rectangle<int> text_area;
    Rectangle<int> value_area;
    const char* text;
    String name;
    Font font {"Carlito Bold", 16.0f, 0};

    int compw;
    String unityofmeasure = "";
    int decimalplaces = 2;
    Rectangle<int> comp_area;

    //std::unique_ptr<ValueLabel> valueLabel;
	std::unique_ptr<ValueLabel2> valueLabel;
    int textsize;
    int lines;
    CompPanelStyle style;
};