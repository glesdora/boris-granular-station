#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>

class BorisSubPanel : public Component
{
public:
    enum SubPanelShape {
        Rounded,
        Plain
    };

    BorisSubPanel(juce::Colour colour, SubPanelShape shape = Rounded);

    void paint(Graphics& g) override;

protected:
    juce::Colour colour;
    SubPanelShape subshape;
};

class BorisSplitSubPanel : public BorisSubPanel
{
public:
    BorisSplitSubPanel(juce::Colour colour, SubPanelShape shape = Rounded, float splitratio = 0.5f, bool splitHorizontally = false);

    void paint(Graphics& g) override;
    void resized();

private:
    Path firstPath;
    Path secondPath;

    float alpha1 = 0.3f;
    float alpha2 = 0.5f;

    float splitratio;
    bool splitHorizontally;
};