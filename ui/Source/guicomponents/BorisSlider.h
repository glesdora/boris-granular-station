#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>
#include "BorisGUIComp.h"
#include "MoonJKnob.h"
#include "MoonJLed.h"

class BorisSlider : public juce::Slider, public BorisGUIComp
{
protected:
    BorisSlider(const String& componentName, float preferredratio);

public:
    double snapValue(double value, DragMode) override;
    void setSkewAndSteps(double skew, int steps);

private:
    double skewFactor = 1.0;
    int numSteps = 101;
};

class BorisVerticalSlider : public juce::Slider, public BorisGUIComp
{
public:
    BorisVerticalSlider(const String& componentName);

    void paint(Graphics& g) override;

    double getValue() override;
};

class BorisVolumeSlider : public BorisVerticalSlider
{
public:
    BorisVolumeSlider(const String& componentName);
};

class BorisHorizontalSlider : public juce::Slider, public BorisGUIComp
{
public:
    BorisHorizontalSlider(const String& componentName);

    void paint(Graphics& g) override;

    double getValue() override;
};

class BorisNumberBoxSlider : public BorisSlider
{
public:
    BorisNumberBoxSlider(const String& componentName);

    void paint(Graphics& g) override;

    ValueLabelPos showValLab() override;
    double getValue() override;
};

class BorisDrfNumBox : public BorisNumberBoxSlider
{
public:
	BorisDrfNumBox(const String& componentName);
};

class BorisDial : public BorisSlider
{
public:
    BorisDial(const String& componentName, int numberOfLeds = 11);

    void paint(Graphics& g) override;
    void resized() override;

    int getNumberOfLeds();
	std::vector<std::unique_ptr<MoonJLed>>& getLeds();
	virtual void litupLeds(float sliderPos);
    Point<int> getCenter();

    double getValue() override;

private:
    int numberOfLeds;

    float rotaryStartAngle = -2.0f;
    float rotaryEndAngle = 2.0f;

    float ledsArcRadius;
    float outerBorderRadius;
    float cursorArcRadius;
    float innerBorderRadius;

    float cursorBandWidth;
    float outerBorderWidth;
    float innerBorderWidth;

	float emptyStartAngle;
	float emptyEndAngle;

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

    friend class BorisLenDial;
};

class BorisDenDial : public BorisDial
{
public:
    BorisDenDial(const String& componentName);

    ValueLabelPos showValLab() override;
};

class BorisFdbDial : public BorisDial
{
public:
    BorisFdbDial(const String& componentName);

    ValueLabelPos showValLab() override;
};

class BorisPtcDial : public BorisDial
{
public:
    BorisPtcDial(const String& componentName);

    double snapValue(double value, DragMode) override;
    double valueToProportionOfLength(double value) override;
    double proportionOfLengthToValue(double proportion) override;
};

class BorisLenDial : public BorisDial
{
public:
    BorisLenDial(const String& componentName);

	void paint(Graphics& g) override;
   
	void valueChanged() override;
	void setTempoMode(bool tempoMode);
    void recalculateMaxValue(double freq);

private:
	double maxvalue = 2000;
	bool tempoMode = false;

    Path inactiveBand;

    void updateInactiveBand();
};

class BorisTmpDial : public BorisDial
{
public:
	BorisTmpDial(const String& componentName, int numberOfLeds);

	void paint(Graphics& g) override;
	void litupLeds(float sliderPos) override;
    void loadIcons(const std::vector<const XmlElement*>&);
	
    ValueLabelPos showValLab() override;
private:
	OwnedArray<Drawable> notes;
};

class BorisMoonJKnobWrapper : public MoonJKnob, public BorisGUIComp
{
public:
    BorisMoonJKnobWrapper(const String& componentName);

    double getValue() override;
    ValueLabelPos showValLab() override;
};

class BorisInvisibleSlider : public juce::Slider, public BorisGUIComp
{
public:
    BorisInvisibleSlider(const String& componentName);

    ValueLabelPos showValLab() override;
    double getValue() override;
};
