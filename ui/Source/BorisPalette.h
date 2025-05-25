#ifndef BORISPALETTE_H
#define BORISPALETTE_H

#pragma once

#include <juce_graphics/juce_graphics.h>

#define LTEXTSIZE 2*24
#define MTEXTSIZE 2*18
#define STEXTSIZE 2*14

enum Palette  { active,
                inactive,
                led,
                offled,
                back,
                ground,
                border,
                ledsband,
                pointer,
                label,
                labeldark,
                contrast,
                button,
                freeze,
                mute
              };

const juce::Colour borisPalette[] = {
	juce::Colours::aliceblue,					// active
	juce::Colours::slategrey,					// inactive
	juce::Colours::aliceblue,					// led
	juce::Colours::darkslategrey.darker(1.8F),  // offled
	juce::Colours::darkslategrey.darker(1.8F),	// back
	juce::Colours::black,	                    // ground
	juce::Colours::darkslateblue,	            // border
    juce::Colours::transparentBlack,			// ledsband
    juce::Colours::aliceblue,					// pointer
    juce::Colours::aliceblue.withAlpha(0.7f),    // label
	juce::Colours::lightslategrey,                   // labeldark
    juce::Colour::fromRGB(180, 128, 255),       // contrast
    juce::Colour::fromRGB(36, 17, 75),          // button
    juce::Colour::fromRGB(89, 202, 252),         // freeze
    juce::Colour::fromRGB(240, 166, 66)            // mute
};

#endif // BORISPALETTE_H