#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>
#include "BorisGUIComp.h"
#include "BorisSlider.h"
#include "BinaryData.h"

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
    ComponentPanel(Component* c, const char* text, int textsize, int lines, String unityofmeasure, int decimalplaces, CompPanelStyle style, const Image& labelimage);


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

	std::unique_ptr<Font> font;

    int compw;
    String unityofmeasure = "";
    int decimalplaces = 2;
    Rectangle<int> comp_area;

    std::unique_ptr<ValueLabel2> valueLabel;
    int textsize;
    int lines;
    CompPanelStyle style;
};