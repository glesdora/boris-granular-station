#ifndef MOONJKNOB_H
#define MOONJKNOB_H

#pragma once

#include <vector>
#include <JuceHeader.h>
#include "MoonJDisplay.h"
#include "MoonJLed.h"
#include "BorisPalette.h"

using namespace juce;

class MoonJKnob : public juce::Slider
{
public:
  MoonJKnob(const String& componentName);
  ~MoonJKnob() override;
  void paint(Graphics& g) override;
  void resized() override;
  void setNumberOfShapes(int n);
  void setData(DoubleBuffer& dbuf);
  void updateShapePath();

private:
  MoonJDisplay display;

  int numberOfShapes = 0;
  int numberOfLeds;
  std::vector<float> leds;
  std::vector<std::unique_ptr<MoonJLed>> ledComponents;

  float rotaryStartAngle = -float_Pi;
  float rotaryEndAngle = float_Pi;

  float ledsArcRadius;
  float outerBorderRadius;
  float cursorArcRadius;
  float innerBorderRadius;

  float cursorBandWidth;
  float outerBorderWidth;
  float innerBorderWidth;

  float ledSize;
  float ledswidth;
  float arrowheight;

  Path borderBand;
  Path arrowBand;
  Path outline;
  Path ledsBand;
  Path arrowPath;

  AffineTransform ledTransform;

  void gaussianIntensity(float p, float d0, std::vector<float>& leds);
};

#endif // MOONJKNOB_H