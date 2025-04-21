#pragma once

#include <vector>
#include <JuceHeader.h>
#include "BorisPalette.h"
#include "DoubleBuffer.h"

using namespace juce;

class MoonJDisplay : public Component
{
public:
    MoonJDisplay();
    ~MoonJDisplay();

    void paint(Graphics& g) override;
	void resized() override;
	void updatePath();
	void setData(DoubleBuffer& dbuf);

private:
	DoubleBuffer* _dbuf = nullptr;
	int _curvesize;
	Path shape;

	Image offscreenImage;
	Image glowedImage;

	GlowEffect glow;
};