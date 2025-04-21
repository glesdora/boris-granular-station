#pragma once

#include <JuceHeader.h>
#include <BorisPalette.h>
#include "BorisGUIComp.h"

class BorisToggle : public juce::ToggleButton, public BorisGUIComp
{
public:
    BorisToggle(const String& buttonName);
    void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    ValueLabelPos showValLab() override;
    double getValue() override;

    void loadIcon(const XmlElement&);
    void setColours(Colour on, Colour off);

private:
    std::unique_ptr<juce::Drawable> icon;
    Colour oncolour = Colours::black;
    Colour offcolour = Colours::black;

	void buttonStateChanged() override
	{
		repaint();
	}
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

class BorisBarToggle : public juce::ToggleButton
{
public:
	BorisBarToggle(const String& buttonName);
	void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	void loadIcon(const XmlElement& xml, float reductiondelta);
	void setColours(Colour on, Colour off);

private:
	Colour oncolour = borisPalette[active];
	Colour offcolour = borisPalette[inactive];
	std::unique_ptr<juce::Drawable> icon;
	float reductiondelta = 0.0f;
};

class BorisRythmToggle : public juce::Component, public BorisGUIComp
{
public:
	class Listener
	{
	public:
		virtual ~Listener() {}
		virtual void toggleChanged(int index) = 0;
	};

	BorisRythmToggle(const String& name, int numToggles);
	void resized() override;
	void mouseDown(const juce::MouseEvent& event) override;

	void loadIcons(const std::vector<const XmlElement*>&);
	void setColours(Colour on, Colour off);

    ValueLabelPos showValLab() override;
	double getValue() override;

	int getCurrentToggleIndex();
	void setActiveToggle(int index);

	void addListener(Listener* listener);
	void removeListener(Listener* listener);

private:
	OwnedArray<BorisBarToggle> toggles;
	int currentToggleIndex = 0;
	ListenerList<Listener> listeners;
};