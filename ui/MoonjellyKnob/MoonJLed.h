#ifndef MOONJLED_H
#define MOONJLED_H

#pragma once

#include <JuceHeader.h>
#include "BorisPalette.h"

using namespace juce;

class MoonJLed : public Component
{
public:
	MoonJLed(bool isLine) : alpha(0.0f), isLine(isLine), ledColour(borisPalette[led])
	{
	}

	void paint(Graphics& g) override
	{
		auto fbounds = getLocalBounds().toFloat();
		float glowradius = 0.0f;

		Image ledim = Image(Image::ARGB, fbounds.getWidth() * 2.0f, fbounds.getHeight() * 2.0f, true);
		Graphics ledig(ledim);
		ledig.addTransform(juce::AffineTransform::translation(fbounds.getWidth() * 0.5f, fbounds.getHeight() * 0.5f));

		if (isLine) {
			glowradius = fbounds.getWidth() * 0.3f;		

			ledig.setColour(borisPalette[offled].withAlpha(1.0f-alpha));
			ledig.fillRect(fbounds);
			ledig.setColour(ledColour.withAlpha(alpha));
			ledig.fillRect(fbounds);
		}
		else {
			glowradius = fbounds.getWidth() * 0.3f;

			ledig.setColour(borisPalette[offled].withAlpha(1.0f - alpha));
			ledig.fillEllipse(fbounds);
			ledig.setColour(borisPalette[led].withAlpha(alpha));
			ledig.fillEllipse(fbounds);
		}

		g.drawImage(ledim, fbounds);

		//Image glowim = ledim.createCopy();
		//Graphics glowig(glowim);

		//GlowEffect glow;
		//glow.setGlowProperties(glowradius * alpha, borisPalette[led].withAlpha(alpha));
		//glow.applyEffect(ledim, glowig, 1.0f, 1.0f);

		//g.drawImage(glowim, fbounds);
	}

	void setBrightness(float a)
	{
		alpha = a;
	}

	bool isLineType() const
	{
		return isLine;
	}

private:
	float alpha = 0.0f;
	const bool isLine;
	bool blue = false;

	Colour ledColour;
};

#endif // MOONJLED_H