#include "MoonJKnob.h"

MoonJKnob::MoonJKnob(const String& componentName) : Slider(componentName), display()
{
    addAndMakeVisible(display);
    
    setSliderStyle(RotaryHorizontalVerticalDrag);
    setTextBoxStyle(NoTextBox, true, 0, 0);
    setRotaryParameters(0, 2*float_Pi, false);
	setRange(0.0, 1.0);
}

MoonJKnob::~MoonJKnob()
{
	setComponentEffect(nullptr);
}

void MoonJKnob::paint(Graphics& g)
{
    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();

    auto sliderPos =  static_cast<float>(valueToProportionOfLength(getValue()));
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour(borisPalette[back]);
    g.fillEllipse(center.x - innerBorderRadius, center.y - innerBorderRadius, innerBorderRadius * 2, innerBorderRadius * 2);

	/*g.setColour(borisPalette[border]);
	g.strokePath(borderBand, PathStrokeType(innerBorderWidth));*/
    
    g.setColour(borisPalette[back]);
    g.strokePath(arrowBand, PathStrokeType(cursorBandWidth));

	g.setColour(borisPalette[back].darker());
	g.strokePath(outline, PathStrokeType(outerBorderWidth));

    // if (borisPalette[ledsband].getAlpha() > 0.0f)
    // {
    //     g.setColour(borisPalette[ledsband]);
    //     g.strokePath(ledsBand, PathStrokeType(ledsbandwidth));
    // }

    Point<float> indicatorPos(
        center.x + std::sin(toAngle) * cursorArcRadius,
        center.y + std::cos(toAngle) * cursorArcRadius * (-1.0f)
    );

	g.setColour(borisPalette[led]);
	g.fillEllipse(
		indicatorPos.x - ledSize * 0.5f,
		indicatorPos.y - ledSize * 0.5f,
		ledSize, ledSize
	);

 //   float zoom = 1.0f;

	//Rectangle<float> arrowBounds(-arrowheight, -arrowheight, arrowheight * 2.0f, arrowheight * 2.0f);
	//Image arrowImage = Image(Image::ARGB, arrowheight * zoom * 2.0f, arrowheight * zoom * 2.0f, true);
 //   Graphics arrowGraphics(arrowImage);
 //   AffineTransform transform = AffineTransform::rotation(toAngle).translated(arrowheight * zoom, arrowheight * zoom);

	//arrowGraphics.setColour(borisPalette[led]);
	//arrowGraphics.fillPath(arrowPath, transform);

    // glow effect
    //float glowradius = 0.1f * arrowheight * zoom;
    //GlowEffect glow;
    //glow.setGlowProperties(glowradius, borisPalette[led]);
    //Image glowedImage = arrowImage.createCopy();
    //Graphics glowGraphics(glowedImage);

    //glow.applyEffect(arrowImage, glowGraphics, 1.0f, 1.0f);
    //g.drawImage(glowedImage, arrowBounds.translated(indicatorPos.x, indicatorPos.y), RectanglePlacement::stretchToFit);

	//g.drawImage(arrowImage, arrowBounds.translated(indicatorPos.x, indicatorPos.y), RectanglePlacement::stretchToFit);

    // leds brightness
    if (numberOfShapes) {
        gaussianIntensity(sliderPos*numberOfLeds, 0.75f, leds);

        for (int i = 0; i < numberOfLeds; ++i) {
			ledComponents[i]->setBrightness(leds[i]);
        }
    }
}

void MoonJKnob::resized()
{
    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();
    Rectangle<int> displaybounds;

    ledsArcRadius = jmin(getWidth(), getHeight()) * 0.4f;
    outerBorderRadius = ledsArcRadius * 0.86f;
    cursorBandWidth = outerBorderRadius * 0.25f;        // Change this to change ratio between cursor band and inner circle

    ledSize = ledsArcRadius * 0.085f;
    innerBorderRadius = outerBorderRadius - cursorBandWidth;
    cursorArcRadius = innerBorderRadius + cursorBandWidth * 0.5f;

    outerBorderWidth = 0.5f;
    innerBorderWidth = outerBorderRadius * 0.026f;

    arrowheight = cursorBandWidth * 0.5f;
    ledswidth = ledSize * 0.5f;

    display.setBounds(bounds.withSizeKeepingCentre(innerBorderRadius * 1.25f, innerBorderRadius * 1.25f));

	borderBand.clear();
	arrowBand.clear();
	outline.clear();
	ledsBand.clear();
	arrowPath.clear();

    borderBand.addCentredArc(center.x, center.y, innerBorderRadius, innerBorderRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    arrowBand.addCentredArc(center.x, center.y, cursorArcRadius, cursorArcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    outline.addCentredArc(center.x, center.y, outerBorderRadius, outerBorderRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    ledsBand.addCentredArc(center.x, center.y, ledsArcRadius, ledsArcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    arrowPath.addTriangle(-arrowheight * 0.5f, arrowheight * 0.5f, arrowheight * 0.5f, arrowheight * 0.5f, 0.0f, -arrowheight * 0.5f);

    if (numberOfShapes) {
        for (int i = 0; i < numberOfLeds; ++i) {
            float angle = rotaryStartAngle + i * (rotaryEndAngle - rotaryStartAngle) / numberOfLeds;

            Point<float> shapePos(
                center.x + std::sin(angle) * (ledsArcRadius),
                center.y + std::cos(angle) * (ledsArcRadius) * (-1.0f)
            );

            ledTransform = AffineTransform::rotation(angle).translated(shapePos.x, shapePos.y);

            MoonJLed* led = ledComponents[i].get();
            Rectangle<int> ledBounds;

			if (!led->isLineType())
                ledBounds = Rectangle<int>(-ledSize * 0.5f, -ledSize * 0.5f, ledSize, ledSize);
			else
                ledBounds = Rectangle<int>(-ledswidth * 0.5f, -ledSize * 2.5f, ledswidth, ledSize * 3.7f);

            ledComponents[i]->setBounds(ledBounds);
            ledComponents[i]->setTransform(ledTransform);
        }
    }   
}

void MoonJKnob::setNumberOfShapes(int n) {
    numberOfShapes = n;
    numberOfLeds = numberOfShapes * round(24.0 / numberOfShapes);
    leds.resize(numberOfLeds);

	ledComponents.clear();
    ledComponents.resize(numberOfLeds);

	for (int i = 0; i < numberOfLeds; ++i) {
        if (i % (numberOfLeds / numberOfShapes)) {
            ledComponents[i].reset(new MoonJLed(false));
		}
		else {
			ledComponents[i].reset(new MoonJLed(true));
		}
		
		addAndMakeVisible(ledComponents[i].get());
    }

    setRange(0.0, (double)numberOfShapes, 0.01);
}

void MoonJKnob::gaussianIntensity(float p, float d0, std::vector<float>& leds) {
    int n = leds.size();

    for (int i = 0; i < n; ++i) {
        float d = jmin(abs(i-p), n-abs(i-p));
        leds[i] = exp(-d*d/(2*d0*d0));
    }
}

void MoonJKnob::setData(DoubleBuffer& dbuf) {
    display.setData(dbuf);
}

void MoonJKnob::updateShapePath() {
	display.updatePath();
}