#include "BorisGUIComp.h"

BorisGUIComp::BorisGUIComp(float preferredratio) : _preferredratio(preferredratio) {}

float BorisGUIComp::getPreferredRatio() { return _preferredratio; }

TextPosition BorisGUIComp::placeText() { return Top; }
ValueLabelPos BorisGUIComp::showValLab() { return Bottom; }
double BorisGUIComp::valueToDisplay() { return getValue(); }

//----------------------------------------------------------------

ValueLabel2::ValueLabel2(BorisGUIComp* c, Value& v, int dp, const String& uom) : value(v), dp(dp), uom(uom)
{
    component = c;
    if (Slider* slider = dynamic_cast<Slider*>(component); slider != nullptr) {
        this->slider = slider;
    }
    value.addListener(this);

	valueChanged(value);
}

void ValueLabel2::valueChanged(juce::Value&) {
    text = "";
    if (prepX) { text = "x "; }

    String stringvalue;
    
    if (uom == " dB")
        stringvalue = String(static_cast<double> (std::log10(double(value.getValue())) * 20.0), dp); //stringvalue = String(Decibels::gainToDecibels(double(value.getValue())), dp);
    else
        stringvalue = String(double(value.getValue()), dp);
    text = stringvalue + uom;

    repaint();
}

void ValueLabel2::paint(Graphics& g)
{
    g.setFont(font);
    g.setColour(borisPalette[labeldark]);
    g.drawText(text, getLocalBounds(), Justification::centred);
}

void ValueLabel2::setUnityOfMeasure(const String& unity) { this->uom = unity; }
void ValueLabel2::setDecimalPlaces(int dp) { this->dp = dp; }
void ValueLabel2::prependX() { prepX = true; }

void ValueLabel2::setTransmitEvents(bool b)
{
    if (slider != nullptr) {
        forwardevents = b;
    }
}

void ValueLabel2::mouseDown(const juce::MouseEvent& event) {
    if (forwardevents) {
        slider->mouseDown(event.getEventRelativeTo(slider));
    }
}

void ValueLabel2::mouseDrag(const juce::MouseEvent& event) {
    if (forwardevents) {
        slider->mouseDrag(event.getEventRelativeTo(slider));
    }
}

void ValueLabel2::mouseUp(const juce::MouseEvent& event) {
    if (forwardevents) {
        slider->mouseUp(event.getEventRelativeTo(slider));
    }
}

void ValueLabel2::mouseDoubleClick(const juce::MouseEvent& event) {
    if (forwardevents) {
        slider->mouseDoubleClick(event.getEventRelativeTo(slider));
    }
}