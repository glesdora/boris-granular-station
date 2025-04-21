#include "BorisSubPanel.h"

BorisSubPanel::BorisSubPanel(juce::Colour colour, SubPanelShape shape) : colour(colour), subshape(shape) {}

void BorisSubPanel::paint(Graphics& g) {
    auto bounds = getLocalBounds();

    g.setColour(colour);
    if (subshape == Rounded) {
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    }
    else {
        g.fillRect(bounds.toFloat());
    }
}

//----------------------------------------------------------------

BorisSplitSubPanel::BorisSplitSubPanel(juce::Colour colour, SubPanelShape shape, float splitratio, bool splitHorizontally)
    : BorisSubPanel(colour, shape), splitratio(splitratio), splitHorizontally(splitHorizontally)
{
}

void BorisSplitSubPanel::paint(Graphics& g) {
    auto bounds = getLocalBounds();

    g.setColour(colour.withAlpha(alpha1));
    g.saveState();
    g.reduceClipRegion(firstPath);
    if (subshape == Rounded)
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    else
        g.fillRect(bounds);
    g.restoreState();

    g.setColour(colour.withAlpha(alpha2));
    g.saveState();
    g.reduceClipRegion(secondPath);
    if (subshape == Rounded)
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    else
        g.fillRect(bounds);
    g.restoreState();
}

void BorisSplitSubPanel::resized() {
    auto bounds = getLocalBounds();
    auto w = bounds.getWidth();
    auto h = bounds.getHeight();

    firstPath.clear();
    secondPath.clear();

    if (!splitHorizontally) {
        firstPath.addRectangle(bounds.withTrimmedRight(w * (1.0f - splitratio)));
        secondPath.addRectangle(bounds.withTrimmedLeft(w * splitratio));
    }
    else {
        firstPath.addRectangle(bounds.withTrimmedBottom(h * (1.0f - splitratio)));
        secondPath.addRectangle(bounds.withTrimmedTop(h * splitratio));
    }
}