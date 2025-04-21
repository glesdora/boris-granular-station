#include "BorisToggle.h"

BorisToggle::BorisToggle(const String& buttonName) : ToggleButton(buttonName), BorisGUIComp(1) {}

void BorisToggle::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds();
    auto w = bounds.getWidth();

    Rectangle<float> buttonArea = bounds.toFloat();
    Rectangle<float> r = buttonArea.reduced(2.0f);

    g.setColour(borisPalette[border]);
    Path p;
    p.addEllipse(r);
    g.strokePath(p, PathStrokeType(2));

    if (icon)
    {
        if (getToggleState()) {
            icon->replaceColour(offcolour, oncolour);
        }
        else {
            icon->replaceColour(oncolour, offcolour);
        }
        icon->drawWithin(g, r.reduced(r.getWidth() * 0.0f), juce::RectanglePlacement::centred, 1.0f);
    }
}

ValueLabelPos BorisToggle::showValLab() { return NoLabel; }

double BorisToggle::getValue() { return getToggleState() ? 1.0 : 0.0; }

void BorisToggle::loadIcon(const XmlElement& xml)
{
	auto state = getToggleState();

	icon = Drawable::createFromSVG(xml);
	icon->replaceColour(Colours::white, borisPalette[button]);
	icon->replaceColour(Colours::black, state ? oncolour : offcolour);
}

void BorisToggle::setColours(Colour on, Colour off)
{
    oncolour = on;
    offcolour = off;

	if (icon)
	{
		auto state = getToggleState();
		icon->replaceColour(Colours::black, state ? oncolour : offcolour);
	}
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

BorisBarToggle::BorisBarToggle(const String& buttonName) : ToggleButton(buttonName)
{
}

void BorisBarToggle::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto bounds = getLocalBounds();
	auto w = bounds.getWidth();

	Rectangle<float> buttonArea = bounds.toFloat();
	Rectangle<float> r = buttonArea.reduced(1.0f);
	auto r_h = r.getHeight();

	g.setColour(borisPalette[border]);
	Path p;
	p.addRoundedRectangle(r, r_h / 2.0f);
	g.strokePath(p, PathStrokeType(1.0f));

	if (icon)
	{
		if (getToggleState()) {
			icon->replaceColour(offcolour, oncolour);
		}
		else {
			icon->replaceColour(oncolour, offcolour);
		}
		icon->drawWithin(g, r.reduced(r_h * reductiondelta), juce::RectanglePlacement::centred, 1.0f);
	}
}

void BorisBarToggle::loadIcon(const XmlElement& xml, float reductiondelta)
{

	icon = juce::Drawable::createFromSVG(xml);
	icon->replaceColour(Colours::black, oncolour);

	this->reductiondelta = reductiondelta;
}

void BorisBarToggle::setColours(Colour on, Colour off)
{
	oncolour = on;
	offcolour = off;
}

BorisRythmToggle::BorisRythmToggle(const String& name, int numToggles) : Component(name), BorisGUIComp(numToggles)
{
	for (int i = 0; i < numToggles; i++) {
		toggles.add(new BorisBarToggle(name + String(i)));
		toggles[i]->setRadioGroupId(1);
		addAndMakeVisible(toggles[i]);
		toggles[i]->setInterceptsMouseClicks(false, false);
	}

	toggles[0]->setToggleState(true, juce::dontSendNotification);
}

void BorisRythmToggle::resized()
{
	auto bounds = getLocalBounds();
	auto w = bounds.getWidth();

	int numToggles = toggles.size();
	int toggleWidth = w / numToggles;

	for (int i = 0; i < numToggles; i++)
	{
		toggles[i]->setBounds(i * toggleWidth, 0, toggleWidth, bounds.getHeight());
	}
}

void BorisRythmToggle::mouseDown(const juce::MouseEvent& e)
{
	auto mousePos = e.getPosition();
	for (int i = 0; i < toggles.size(); ++i) {
		if (toggles[i]->getBounds().contains(mousePos)) {
			toggles[i]->setToggleState(true, juce::dontSendNotification);
			currentToggleIndex = i;
			listeners.call([i](Listener& l) { l.toggleChanged(i); });
			break;
		}
	}
}

void BorisRythmToggle::loadIcons(const std::vector<const XmlElement*>& iconsXmls)
{

	float reddeltas[] = { 0.1f, 0.4f, 0.2f };

	for (int i = 0; i < iconsXmls.size(); i++) {
		toggles[i]->loadIcon(*iconsXmls[i], reddeltas[i]);
	}
}

void BorisRythmToggle::setColours(Colour on, Colour off)
{
	for (int i = 0; i < toggles.size(); i++) {
		toggles[i]->setColours(on, off);
	}
}

ValueLabelPos BorisRythmToggle::showValLab() { return NoLabel; }
double BorisRythmToggle::getValue() { return static_cast<double>(getCurrentToggleIndex()); }

int BorisRythmToggle::getCurrentToggleIndex()
{
	return currentToggleIndex;
}

void BorisRythmToggle::setActiveToggle(int index)
{
	currentToggleIndex = index;
	toggles[index]->setToggleState(true, juce::sendNotification);
}

void BorisRythmToggle::addListener(Listener* listener)
{
	listeners.add(listener);
}

void BorisRythmToggle::removeListener(Listener* listener)
{
	listeners.remove(listener);
}