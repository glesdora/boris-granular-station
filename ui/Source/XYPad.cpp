#include "XYPad.h"

XYPad::Thumb::Thumb() : thumbColour(juce::Colours::black)
{
    constrainer.setMinimumOnscreenAmounts(thumbSize, thumbSize, thumbSize, thumbSize);
}

void XYPad::Thumb::paint(juce::Graphics& g)
{
    g.setColour(thumbColour);
    g.drawEllipse(getLocalBounds().reduced(2).toFloat(), 2.0f);
}

void XYPad::Thumb::setThumbColour(Colour colour)
{
    thumbColour = colour;
    repaint();
}

void XYPad::Thumb::mouseDown(const juce::MouseEvent& event)
{
    dragger.startDraggingComponent(this, event);
}

void XYPad::Thumb::mouseDrag(const juce::MouseEvent& event)
{
    juce::MessageManagerLock lock;

    dragger.dragComponent(this, event, &constrainer);
    if (moveCallback) {
        //moveCallback(getPosition().toDouble());
		moveCallback(getPosition().toDouble());
    }
}

XYPad::XYPad() : borderColour(juce::Colours::darkgrey.darker(1.8F))
{
    addAndMakeVisible(thumb);
	thumb.setBufferedToImage(true);

    // Set the callback for the thumb to move the sliders
    thumb.moveCallback = [&] (Point<double> position)->void {
        const std::lock_guard<std::mutex> lock(vectorMutex);
        const auto bounds = getLocalBounds().toDouble();
        const auto w = static_cast<double>(thumbSize);
		if (xSlider != nullptr) {
            xSlider->setValue(jmap(position.getX(), 0.0, bounds.getWidth() - w, xSlider->getMaximum(), xSlider->getMinimum()));
        }           //inverted the output mapping, cause the pos slider works "in reverse"
		if (ySlider != nullptr) {
            ySlider->setValue(roundToInt(jmap(position.getY(), bounds.getHeight() - w, 0.0, ySlider->getMinimum(), ySlider->getMaximum())));
        }
    };
}

void XYPad::paint(juce::Graphics& g)
{
	auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colours::transparentBlack);
    g.fillRect(bounds);

    g.setColour(borderColour);
	g.drawRect(bounds, 2.0f);
}

void XYPad::setColours(const Colour& thumbColour, const Colour& borderColour)
{
    thumb.setThumbColour(thumbColour);
	this->borderColour = borderColour;
}

void XYPad::resized()
{
    thumb.setBounds(getLocalBounds().withSizeKeepingCentre(thumbSize, thumbSize));
}

void XYPad::registerSlider(juce::Slider* slider, Axis axis)
{
    if ((axis == Axis::X && xSlider != nullptr) || (axis == Axis::Y && ySlider != nullptr))
    {
        return;
    }

    slider->addListener(this);

    if (axis == Axis::X)
        xSlider = slider;
    else
        ySlider = slider;

}

void XYPad::deregisterSlider(juce::Slider* slider)
{
    if (xSlider == slider)
    {
        xSlider->removeListener(this);
        xSlider = nullptr;
    }
    else if (ySlider == slider)
    {
        ySlider->removeListener(this);
        ySlider = nullptr;
    }
}

void XYPad::initX(double value)
{
    thumb.setTopLeftPosition(
        jmap(value, 0., 1., getLocalBounds().getWidth() - static_cast<double>(thumbSize), 0.0),
        thumb.getY());     //inverted the output mapping, cause the pos slider works "in reverse"
}

void XYPad::initY(double value)
{
    thumb.setTopLeftPosition(
        thumb.getX(),
        jmap(value, 0., 1., getLocalBounds().getHeight() - static_cast<double>(thumbSize), 0.0));
}

void XYPad::mouseDown(const juce::MouseEvent& event)
{
    //as the user clicked on the XYPad, we want to move the thumb to the clicked position
    thumb.setCentrePosition(Point<int>(event.getPosition()));
    thumb.mouseDown(event);
    if (thumb.moveCallback) {
        thumb.moveCallback(thumb.getPosition().toDouble());
    }
}

void XYPad::mouseDrag(const juce::MouseEvent& event)
{
    thumb.mouseDrag(event);
}

void XYPad::sliderValueChanged(juce::Slider* slider)
{
    if (thumb.isMouseOverOrDragging()) return;

	auto value = slider->getValue();
	auto min = slider->getMinimum();
	auto max = slider->getMaximum();

	if (slider == xSlider) {
		updateThumbPosition(Axis::X, value, min, max);
	}
	else if (slider == ySlider) {
		updateThumbPosition(Axis::Y, value, min, max);
	}
}

void XYPad::updateThumbPosition(Axis axis, double value, double min, double max)
{
	//DBG("Update thumb position for axis " << ( axis ? "Y" : "X") << ": " << value);

	const std::lock_guard<std::mutex> lock(vectorMutex);
	const auto bounds = getLocalBounds().toDouble();
	const auto w = static_cast<double>(thumbSize);
	if (axis == Axis::X) {
		thumb.setTopLeftPosition(
			jmap(value, min, max, bounds.getWidth() - w, 0.0),
			thumb.getY());     //inverted the output mapping, cause the pos slider works "in reverse"
	}
	else {
		thumb.setTopLeftPosition(
			thumb.getX(),
			jmap(value, min, max, bounds.getHeight() - w, 0.0));
	}
}

bool XYPad::getControlledByMouse() const
{
    return (thumb.isMouseButtonDown() || this->isMouseButtonDown());
}