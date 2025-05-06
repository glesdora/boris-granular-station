#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>

using namespace juce;

class BorisLogo : public Component
{
public: 
    BorisLogo() {}
    
    void paint(Graphics& g) {
        auto bounds = getLocalBounds();

        Rectangle<int> topArea = bounds.removeFromTop(bounds.getHeight() * 0.66f);
        Rectangle<int> bottomArea = bounds;

        g.setColour(Colours::white);

        // g.drawRect(topArea);
        // g.drawRect(bottomArea);

        g.setFont(font);
        g.drawText("BORIS", topArea, Justification::centredTop);
        g.setFont(24.0f);
        //g.setColour(Colours::lightpink);
        g.drawText("GRANULAR STATION", bottomArea, Justification::centredTop);
    }

private:
    juce::Font font {"Carlito", 88.0f, 0};
};