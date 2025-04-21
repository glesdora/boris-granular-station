#include "MoonJDisplay.h"

MoonJDisplay::MoonJDisplay()
{
}

MoonJDisplay::~MoonJDisplay()
{
	_dbuf = nullptr;
}

void MoonJDisplay::paint(Graphics& g)
{
	if (_dbuf == nullptr) return;

    auto bounds = getLocalBounds();
	float linethickness = _curvesize * 0.05f;
	float glowradius = linethickness * 0.5f;
	float margin = linethickness + glowradius;
	auto displayarea = bounds.reduced(margin);
	auto w = displayarea.getWidth();
	auto h = displayarea.getHeight();

    g.setColour(borisPalette[back]);
    g.fillRect(displayarea);

    offscreenImage = Image(Image::ARGB, _curvesize + 2 * margin, _curvesize + 2 * margin, true);
    Graphics offscreenG(offscreenImage);
    offscreenG.addTransform(juce::AffineTransform::translation(margin, margin));

    offscreenG.setColour(borisPalette[led]);
    offscreenG.strokePath(shape, juce::PathStrokeType(linethickness, PathStrokeType::mitered, PathStrokeType::rounded));

    glow.setGlowProperties(glowradius, borisPalette[led]);
    glowedImage = offscreenImage.createCopy();
    Graphics glowedGraphics(glowedImage);
    glow.applyEffect(offscreenImage, glowedGraphics, 1.0f, 1.0f);

    g.drawImage(glowedImage, bounds.toFloat(), juce::RectanglePlacement::stretchToFit);
}

void MoonJDisplay::resized()
{
	updatePath();
}

void MoonJDisplay::updatePath() {
    auto bounds = getLocalBounds();
    auto w = bounds.getWidth();
    auto h = bounds.getHeight();

    shape.clear();

    if (_dbuf != nullptr) {
        auto& _shapeData = _dbuf->getActiveBuffer();
		int shapesize = _shapeData.size();

		shape.startNewSubPath(0, shapesize - _shapeData[0] * shapesize);
        for (int i = 1; i < shapesize; i++) {
            shape.lineTo(i, shapesize - _shapeData[i] * shapesize);
        }
    }

	//// Debug representation of the shape: no connecting points with straight lines, but "sample and hold" style
	//shape.startNewSubPath(bounds.getX(), bounds.getBottom());
	//if (_dbuf != nullptr) {
	//	auto& _shapeData = _dbuf->getActiveBuffer();
	//	for (int i = 0; i < _shapeData.size(); i++) {
	//		shape.lineTo(bounds.getX() + i * w / _shapeData.size(), bounds.getBottom() - _shapeData[i] * h);
	//		shape.lineTo(bounds.getX() + (i + 1) * w / _shapeData.size(), bounds.getBottom() - _shapeData[i] * h);
	//	}
	//}

	//shape.lineTo(bounds.getRight(), bounds.getBottom());
}

void MoonJDisplay::setData(DoubleBuffer& dbuf) {
	_dbuf = &dbuf;
	_curvesize = _dbuf->getActiveBuffer().size();
}