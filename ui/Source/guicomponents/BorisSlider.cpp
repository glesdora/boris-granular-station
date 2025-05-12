#include "BorisSlider.h"

BorisSlider::BorisSlider(const String& componentName, float preferredratio) : Slider(componentName), BorisGUIComp(preferredratio) {}

double BorisSlider::snapValue(double value, DragMode)
{
    double minValue = getMinimum();
    double maxValue = getMaximum();

    double normalizedPos = std::pow((value - minValue) / (maxValue - minValue), skewFactor);
    double stepSize = 1.0 / (numSteps - 1);
    double snappedPos = std::round(normalizedPos / stepSize) * stepSize;

    return minValue + (maxValue - minValue) * std::pow(snappedPos, 1.0 / skewFactor);
}

void BorisSlider::setSkewAndSteps(double skew, int steps)
{
    setSkewFactor(skew);

    skewFactor = skew;
    numSteps = steps;
}

//-----------------------------------------------------------------------------------------------------------------------------------

BorisVerticalSlider::BorisVerticalSlider(const String& componentName) : Slider(componentName), BorisGUIComp(0.125f)
{
    setSliderStyle(SliderStyle::LinearBarVertical);
    setTextBoxStyle(NoTextBox, true, 0, 0);
}

void BorisVerticalSlider::paint(Graphics& g)
{
    auto bounds = getLocalBounds();
    auto sliderPos = bounds.getHeight() - getPositionOfValue(getValue());
    auto cornersize = bounds.getWidth() * 0.25f;

    Path roundedRect;
    roundedRect.addRoundedRectangle(bounds, cornersize);

    g.setColour(borisPalette[inactive]);        // Or replace with colour1
    g.fillPath(roundedRect);

    Path activePath;
    activePath.addRoundedRectangle(bounds.removeFromBottom(sliderPos), cornersize * 0.5f);

    g.setColour(borisPalette[active]);
    g.saveState();
    g.reduceClipRegion(activePath);
    g.fillPath(roundedRect);
    g.restoreState();
}

double BorisVerticalSlider::getValue() { return Slider::getValue(); }

//-----------------------------------------------------------------------------------------------------------------------------------

BorisVolumeSlider::BorisVolumeSlider(const String& componentName) : BorisVerticalSlider(componentName) {}

//-----------------------------------------------------------------------------------------------------------------------------------

BorisHorizontalSlider::BorisHorizontalSlider(const String& componentName) : Slider(componentName), BorisGUIComp(8)
{
    setSliderStyle(SliderStyle::LinearBar);
    setTextBoxStyle(NoTextBox, true, 0, 0);
}

void BorisHorizontalSlider::paint(Graphics& g)
{
    auto bounds = getLocalBounds();
    auto sliderPos = getPositionOfValue(getValue());
    auto cornersize = bounds.getHeight() * 0.25f;

    Path roundedRect;
    roundedRect.addRoundedRectangle(bounds, cornersize);

    g.setColour(borisPalette[inactive]);
    g.fillPath(roundedRect);

    Path activePath;
    activePath.addRoundedRectangle(bounds.removeFromLeft(sliderPos), cornersize * 0.5f);

    g.setColour(borisPalette[active]);
    g.saveState();
    g.reduceClipRegion(activePath);
    g.fillPath(roundedRect);
    g.restoreState();
}

double BorisHorizontalSlider::getValue() { return Slider::getValue(); }

//-----------------------------------------------------------------------------------------------------------------------------------

BorisNumberBoxSlider::BorisNumberBoxSlider(const String& componentName) : BorisSlider(componentName, 4)
{
    setSliderStyle(SliderStyle::LinearVertical);
    setTextBoxStyle(TextBoxBelow, false, 0, 0);
    setSliderSnapsToMousePosition(false);

    setSkewAndSteps(1.0, 101);
}

void BorisNumberBoxSlider::paint(Graphics& g)
{
    auto bounds = getLocalBounds();
    auto sliderPos = bounds.getHeight() - getPositionOfValue(getValue());
    auto cornersize = bounds.getWidth() * 0.05f;

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

ValueLabelPos BorisNumberBoxSlider::showValLab() { return Front; }

double BorisNumberBoxSlider::getValue() { return Slider::getValue(); }

//-----------------------------------------------------------------------------------------------------------------------------------

BorisDrfNumBox::BorisDrfNumBox(const String& componentName) : BorisNumberBoxSlider(componentName) {}

//-----------------------------------------------------------------------------------------------------------------------------------

BorisDial::BorisDial(const String& componentName, int numberOfLeds) : BorisSlider(componentName, 1.3f), numberOfLeds(numberOfLeds)
{
    setSliderStyle(SliderStyle::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(NoTextBox, true, 0, 0);

    led_ptrs.resize(numberOfLeds);
    for (int i = 0; i < numberOfLeds; i++) {
        led_ptrs[i].reset(new MoonJLed(false));
        addAndMakeVisible(led_ptrs[i].get());
    }
}

void BorisDial::paint(Graphics& g)
{
    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();
    center.addXY(0, leds_to_in_rad * 0.5f);

	g.drawEllipse(center.x - 1, center.y - 1, 2, 2, 2.0f);

    auto sliderPos = static_cast<float>(valueToProportionOfLength(getValue()));
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

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

	litupLeds(sliderPos);
}

void BorisDial::resized()
{
    auto bounds = getLocalBounds();

    float boundsratio = static_cast<float>(bounds.getWidth()) / static_cast<float>(bounds.getHeight());
    if (boundsratio > getPreferredRatio()) {
        bounds.setWidth(bounds.getHeight() * getPreferredRatio());
    }
    else {
        bounds.setHeight(bounds.getWidth() / getPreferredRatio());
    }

    ledsArcRadius = bounds.getWidth() * 0.45f;

    float outertoledratio = 0.86f;
    float bandcircleratio = 1.0f - (2 / getPreferredRatio() - 1.0f) / outertoledratio;

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

    borderBand.addCentredArc(center.x, center.y, innerBorderRadius, innerBorderRadius, 0.0f, 0.0f, 2 * float_Pi, true);
    outline.addCentredArc(center.x, center.y, outerBorderRadius, outerBorderRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    ledsBand.addCentredArc(center.x, center.y, ledsArcRadius, ledsArcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    arrowPath.addTriangle(-arrowheight * 0.5f, arrowheight * 0.5f, arrowheight * 0.5f, arrowheight * 0.5f, 0.0f, -arrowheight * 0.5f);

    emptyStartAngle = rotaryStartAngle - 1.5f * arrowheight / outerBorderRadius;
    emptyEndAngle = rotaryEndAngle + 1.5f * arrowheight / outerBorderRadius;

    emptyCursorBand.clear();
    emptyCursorBand.addCentredArc(center.x, center.y, cursorArcRadius + cursorBandWidth * 0.5f, cursorArcRadius + cursorBandWidth * 0.5f, 0.0f, emptyStartAngle, emptyEndAngle, true);
    emptyCursorBand.addCentredArc(center.x, center.y, cursorArcRadius - cursorBandWidth * 0.5f, cursorArcRadius - cursorBandWidth * 0.5f, 0.0f, emptyEndAngle, emptyStartAngle, false);
    emptyCursorBand.closeSubPath();

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

int BorisDial::getNumberOfLeds() { return numberOfLeds; }

std::vector<std::unique_ptr<MoonJLed>>& BorisDial::getLeds() { return led_ptrs; }

Point<int> BorisDial::getCenter()
{
    auto center = getLocalBounds().getCentre();
    center.addXY(0, leds_to_in_rad * 0.5f);

    return center;
}

void BorisDial::litupLeds(float sliderPos)
{
    if (led_ptrs.size() == numberOfLeds) {
        int pointedLed = round(sliderPos * (numberOfLeds - 1));
        for (int i = 0; i <= pointedLed; i++) {
            led_ptrs[i]->setBrightness((i + 1) / ((float)(pointedLed + 1)));
        }
        for (int i = pointedLed + 1; i < numberOfLeds; i++) {
            led_ptrs[i]->setBrightness(0.0f);
        }
    }
}

double BorisDial::getValue() { return Slider::getValue(); }

//-----------------------------------------------------------------------------------------------------------------------------------

BorisDenDial::BorisDenDial(const String& componentName) : BorisDial(componentName) {}

ValueLabelPos BorisDenDial::showValLab() { return NoLabel; }

//-----------------------------------------------------------------------------------------------------------------------------------

BorisFdbDial::BorisFdbDial(const String& componentName) : BorisDial(componentName) {}

ValueLabelPos BorisFdbDial::showValLab() { return Front; }

//-----------------------------------------------------------------------------------------------------------------------------------

BorisPtcDial::BorisPtcDial(const String& componentName) : BorisDial(componentName) {}

double BorisPtcDial::snapValue(double value, DragMode)
{
    int subs = 4;

    double semitoneRatio = std::pow(2.0, 1.0 / (12.0 * subs));
    double n = std::round(std::log(value) / std::log(semitoneRatio));

    return std::pow(semitoneRatio, n);
}

double BorisPtcDial::valueToProportionOfLength(double value)
{
    double minValue = getMinimum();
    double maxValue = getMaximum();
    return (std::log(value / minValue)) / (std::log(maxValue / minValue));
}

double BorisPtcDial::proportionOfLengthToValue(double proportion)
{
    double minValue = getMinimum();
    double maxValue = getMaximum();
    return minValue * std::pow(maxValue / minValue, proportion);
}

//-----------------------------------------------------------------------------------------------------------------------------------

BorisLenDial::BorisLenDial(const String& componentName) : BorisDial(componentName)
{
    setSkewAndSteps(0.4203094, 201);
}

void BorisLenDial::paint(Graphics& g)
{
	auto bounds = getLocalBounds();
    auto center = this->getCenter();

    if (tempoMode) {
        g.setColour(borisPalette[inactive]);
        g.fillPath(inactiveBand);
    }

    BorisDial::paint(g);
}

void BorisLenDial::valueChanged()
{
	if (getValue() > maxvalue && tempoMode) {
        setValue(maxvalue);
	}
}

void BorisLenDial::setTempoMode(bool tempoMode)
{
    this->tempoMode = tempoMode;

    repaint();

	if (tempoMode)
		valueChanged();
}

void BorisLenDial::recalculateMaxValue(double freq)
{
	if (freq) {
		maxvalue = 16000 / freq;

        if (maxvalue < getMaximum())
            updateInactiveBand();
        else
            inactiveBand.clear();

        repaint();

        if (tempoMode)
            valueChanged();
	}
}

void BorisLenDial::updateInactiveBand()
{
    auto minValue = getMinimum();
    auto maxValue = getMaximum();
    auto skew = this->getSkewFactor();

    auto rotaryParameters = this->getRotaryParameters();
    auto startAngle = this->rotaryStartAngle;
    auto endAngle = this->rotaryEndAngle;

    auto normMaxPos = std::pow((maxvalue - minValue) / (maxValue - minValue), skew);
    auto maxAngle = startAngle + normMaxPos * (endAngle - startAngle);

    auto center = this->getCenter();
    auto bandEndAngle = this->emptyEndAngle;
    auto bandRadius = this->cursorArcRadius;
    auto bandWidth = this->cursorBandWidth;

    inactiveBand.clear();
    inactiveBand.addCentredArc(center.x, center.y, bandRadius + 0.33f * bandWidth, bandRadius + 0.33f * bandWidth, 0.0f, maxAngle, bandEndAngle, true);
    inactiveBand.addCentredArc(center.x, center.y, bandRadius - 0.33f * bandWidth, bandRadius - 0.33f * bandWidth, 0.0f, bandEndAngle, maxAngle, false);
    inactiveBand.closeSubPath();
}

//-----------------------------------------------------------------------------------------------------------------------------------

BorisTmpDial::BorisTmpDial(const String& componentName, int nol) : BorisDial(componentName, nol)
{
    setSkewAndSteps(1.0, 7);
}

void BorisTmpDial::paint(Graphics& g)
{
    auto tallnotesbounds = getLocalBounds().reduced(getWidth() * 0.27f).withCentre(getCenter()).toFloat();
	auto shortnotesbounds = getLocalBounds().reduced(getWidth() * 0.35f).withCentre(getCenter()).toFloat();
    auto value = static_cast<int>(getValue());

    BorisDial::paint(g);

    if (notes.size() == getNumberOfLeds()) {
        if (value == 6)
			notes[value]->drawWithin(g, shortnotesbounds, RectanglePlacement::centred, 0.75f);
        else
            notes[value]->drawWithin(g, tallnotesbounds, RectanglePlacement::centred, 0.75f);

    }
}

void BorisTmpDial::litupLeds(float sliderPos)
{
	auto tempoNumberOfLeds = getNumberOfLeds();
    auto& tempo_led_ptrs = getLeds();

	if (tempo_led_ptrs.size() == tempoNumberOfLeds) {
		int pointedLed = round(sliderPos * (tempoNumberOfLeds - 1));
		for (int i = 0; i < tempoNumberOfLeds; i++) {
			tempo_led_ptrs[i]->setBrightness(static_cast<float>(i == pointedLed));
		}
	}
}

ValueLabelPos BorisTmpDial::showValLab() { return NoLabel; }

void BorisTmpDial::loadIcons(const std::vector<const XmlElement*>& iconsXmls)
{
	for (int i = 0; i < iconsXmls.size(); i++) {
        notes.add(juce::Drawable::createFromSVG(*iconsXmls[i]));
		notes[i]->replaceColour(Colours::black, borisPalette[led]);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------

BorisMoonJKnobWrapper::BorisMoonJKnobWrapper(const String& componentName) : MoonJKnob(componentName), BorisGUIComp(1) {}

double BorisMoonJKnobWrapper::getValue() { return Slider::getValue(); }

ValueLabelPos BorisMoonJKnobWrapper::showValLab() { return NoLabel; }

//-----------------------------------------------------------------------------------------------------------------------------------

BorisInvisibleSlider::BorisInvisibleSlider(const String& componentName) : Slider(componentName), BorisGUIComp(0)
{
}

ValueLabelPos BorisInvisibleSlider::showValLab() { return NoLabel; }

double BorisInvisibleSlider::getValue() { return Slider::getValue(); }