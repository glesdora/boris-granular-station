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

	const auto bounds = getLocalBounds().toFloat();
	const int W = static_cast<int>(bounds.getWidth());
	const int H = static_cast<int>(bounds.getHeight());
	const float thickness = 3.f;

	const int sourceSize = _curvesize;
	const float glowRadius = thickness;

	Image maskImage(Image::ARGB, sourceSize, sourceSize, true);
	Graphics maskG(maskImage);

	DropShadow shadow;
	shadow.colour = borisPalette[led]/*.withAlpha(0.6f)*/;
	shadow.radius = (int)glowRadius * 0.5;
	shadow.offset = { 0, 0 }; // No offset for glow effect
	shadow.drawForPath(maskG, shape);
	Path shorterShape = shape;
	shorterShape.applyTransform(AffineTransform::scale(1.0f, 0.9f));
	shorterShape.applyTransform(AffineTransform::translation(0, 0.15f * sourceSize));
	shadow.colour = borisPalette[back];
	shadow.radius = (int)glowRadius;
	shadow.offset = { 0, 0 }; // No offset for glow effect
	shadow.drawForPath(maskG, shorterShape);

	g.drawImage(maskImage, bounds.toFloat(), RectanglePlacement::stretchToFit, false);
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

void MoonJDisplay::drawBlurredPathGlow(Graphics& g, const Path& shape, const Colour& glowColour,
    float lineThickness, float glowRadius, Rectangle<int> bounds)
{
    float glowScale = 0.5f; // 1/4 resolution
    int glowW = static_cast<int>(_curvesize * glowScale);
    int glowH = static_cast<int>(_curvesize * glowScale);

    if (glowW <= 0 || glowH <= 0) return;

    // Step 1: Create low-res glow image
    Image glowImage(Image::ARGB, glowW, glowH, true);
    Graphics glowG(glowImage);

    glowG.addTransform(AffineTransform::scale(glowScale)); // draw shape scaled down
    glowG.setColour(glowColour.withAlpha(0.6f));           // semi-transparent glow color
    glowG.strokePath(shape, PathStrokeType(lineThickness));

    // Step 2: Scale up and draw as glow
    g.drawImage(glowImage, bounds.toFloat());

    // Step 3: Draw sharp main shape on top
    //g.setColour(borisPalette[led]);
    //g.strokePath(shape, PathStrokeType(lineThickness));

}