#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>
#include "BorisGUIComp.h"
#include "BorisSlider.h"

enum CompPanelStyle
{
    Invisible,
    TextDialValue,
    TextDial,
    TextSliderValue,
    TextNumBox,
    LeftTextNumBox,
    RoundToggle,
    TriTab
};

class ComponentPanel : public Component
{
public:
    ComponentPanel(Component* c, const char* text, int textsize, int lines, String unityofmeasure, int decimalplaces, CompPanelStyle style);

    void paint(Graphics& g);
    void resized();

    void setBounds(Rectangle<int> bounds, int compw);
    void setBounds(Rectangle<int> bounds);

private:
    Component* component;
    Rectangle<int> text_area;
    Rectangle<int> value_area;
    const char* text;
    String name;
    Font font{ "Carlito Bold", 16.0f, 0 };

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