#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>

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
    BorisGUIComp(float preferredratio);
    float getPreferredRatio();
    virtual TextPosition placeText();
    virtual ValueLabelPos showValLab();
    virtual double getValue() = 0;
    virtual double valueToDisplay();

private:
    const float _preferredratio;
};


class ValueLabel2 : public Component, public Value::Listener
{
public:
    ValueLabel2(BorisGUIComp* c, Value& v, int dp, const String& uom);

    void valueChanged(juce::Value&) override;

    void paint(Graphics& g) override;

    void setUnityOfMeasure(const String& unity);
    void setDecimalPlaces(int dp);
    void prependX();

    void setTransmitEvents(bool b);
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

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