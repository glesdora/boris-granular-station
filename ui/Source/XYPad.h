#pragma once

#include <JuceHeader.h>

class XYPad : public juce::Component, juce::Slider::Listener
{
public:
    enum Axis { X, Y };

    class Thumb : public juce::Component
    {
    public:
        Thumb();

        void paint(juce::Graphics& g) override;
        void setThumbColour(Colour colour);
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        std::function<void(Point<double>)> moveCallback;

    private:
        Colour thumbColour;
        ComponentDragger dragger;
        ComponentBoundsConstrainer constrainer;
    };

    //==============================================================================
    XYPad();

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void setColours(const Colour& thumbColour, const Colour& borderColour);
    void registerSlider(juce::Slider* slider, Axis axis);
    void deregisterSlider(juce::Slider* slider);

    void initX(double value);
	void initY(double value);

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    void updateThumbPosition(Axis axis, double value, double min, double max);

    bool getControlledByMouse() const;

private:
    void sliderValueChanged(juce::Slider* slider) override;

    static const int thumbSize = 13;

    //std::vector<juce::Slider*> xSliders, ySliders;
    juce::Slider* xSlider = nullptr;
    juce::Slider* ySlider = nullptr;

    Thumb thumb;

    std::mutex vectorMutex;

    Colour borderColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad);
};