#include "ComponentPanel.h"

ComponentPanel::ComponentPanel(Component* c, const char* text, int textsize, int lines, String unityofmeasure, int decimalplaces, CompPanelStyle style)
    : component(c), text(text), textsize(textsize), lines(lines), unityofmeasure(unityofmeasure), decimalplaces(decimalplaces), style(style)
{
    name = String(String::fromUTF8(text));
    addAndMakeVisible(component);

    if (auto slider = dynamic_cast<Slider*>(component); slider != nullptr) {
        valueLabel.reset(new ValueLabel2(dynamic_cast<BorisGUIComp*>(component), slider->getValueObject(), decimalplaces, unityofmeasure));
        addAndMakeVisible(*valueLabel.get());
    }

    static auto typeface = Typeface::createSystemTypefaceFor(BinaryData::Carlito_Bold_SUBSET_ttf, BinaryData::Carlito_Bold_SUBSET_ttf_Size);

    if (typeface != nullptr) {
        font.reset(new Font(typeface));
        font->setHeight(textsize);
    }
}

void ComponentPanel::paint(Graphics& g) {      
	//g.setColour(Colours::olive);
	//g.drawRect(getLocalBounds(), 2);

 //   g.setColour(Colours::purple);
	//g.drawRect(comp_area, 3);

	//g.setColour(Colours::gold);
	//g.drawRect(value_area, 3);

    g.setFont(*font);
    g.setFont(textsize);
    g.setColour(Colours::aliceblue.withAlpha(0.7f));

    if (style == LeftTextNumBox)
        g.drawFittedText(name, text_area, Justification::centredRight, 2, 1.0f);
    else
        g.drawFittedText(name, text_area, Justification::centredTop, 2, 1.0f);
}

void ComponentPanel::resized() {
    auto bounds = getLocalBounds();

    if (BorisGUIComp* c = dynamic_cast<BorisGUIComp*>(component); c != nullptr) {
        auto valLabPos = c->showValLab();

        if (style == TextNumBox) {
            Rectangle<int> textband;
            Rectangle<int> compbounds;

            int cushion = 2;            // pixels between text and component

            float prefratio = c->getPreferredRatio();
            float comph = compw / prefratio;

            if (bounds.getHeight() >= comph && bounds.getWidth() >= compw) {
                compbounds = bounds.removeFromBottom(comph);
                compbounds = compbounds.withSizeKeepingCentre(compw, comph);
                bounds.removeFromBottom(cushion);
                textband = bounds.removeFromBottom(textsize * lines);

                comp_area = compbounds;
                text_area = textband;

                valueLabel->setUnityOfMeasure(unityofmeasure);
                valueLabel->setTransmitEvents(true);

                component->setBounds(comp_area);
                valueLabel->setBounds(comp_area);
            }
        }
        else if (style == LeftTextNumBox) {
            Rectangle<int> textband;
            Rectangle<int> compbounds;

            int cushion = 4;            // pixels between text and component

            float prefratio = c->getPreferredRatio();
            float comph = compw / prefratio;

            if (bounds.getHeight() >= comph && bounds.getWidth() >= compw) {
                compbounds = bounds.removeFromRight(compw);
                compbounds = compbounds.withSizeKeepingCentre(compw, comph);
                bounds.removeFromRight(cushion);
                textband = bounds;

                comp_area = compbounds;
                text_area = textband;

                valueLabel->setUnityOfMeasure(unityofmeasure);
                valueLabel->setTransmitEvents(true);

                component->setBounds(comp_area);
                valueLabel->setBounds(comp_area);
            }

        }
        else if (style == RoundToggle) {
            Rectangle<int> compbounds;

            float prefratio = c->getPreferredRatio();
            float comph = compw / prefratio;

            if (bounds.getHeight() >= comph && bounds.getWidth() >= compw) {
                compbounds = bounds.withSizeKeepingCentre(compw, comph);
                comp_area = compbounds;

                component->setBounds(comp_area);
            }
        }
        else if (style == TriTab) {
            /*Rectangle<int> compbounds;

            float prefratio = c->getPreferredRatio();
            float comph = compw / prefratio;

            if (bounds.getHeight() >= comph && bounds.getWidth() >= compw) {
                compbounds = bounds.withSizeKeepingCentre(compw, comph);
                comp_area = compbounds;

                component->setBounds(comp_area);
            }*/

			auto compbounds = bounds.withSizeKeepingCentre(compw, bounds.getHeight());      //ignoring the aspect ratio here, I'll fix it then
			component->setBounds(compbounds);
        }
        else if (style == Invisible) {
        }
        else if (style == TextDialValue || style == TextDial) {
            Rectangle<int> textband;
            if (c->placeText() == Top) {
                auto bheight = textsize * lines;
                textband = bounds.removeFromTop(bheight);
            }
            else if (c->placeText() == Left) {
                textband = bounds.removeFromLeft(textsize * 2.0f);
            }

            //Rectangle<int> bottomband;
            //if (valLabPos == Bottom) {
            //    bottomband = bounds.removeFromBottom(STEXTSIZE * 1.2f);
            //}

            Rectangle<int> comp_bounds = bounds;
            float panelratio = static_cast<float>(comp_bounds.getWidth()) / static_cast<float>(comp_bounds.getHeight());
            float prefratio = c->getPreferredRatio();

            if (prefratio) {
                auto bot = comp_bounds.getBottom();

                if (panelratio < prefratio) {
                    comp_bounds = comp_bounds.withSizeKeepingCentre(comp_bounds.getWidth(), comp_bounds.getWidth() / prefratio);
                }
                else {
                    comp_bounds = comp_bounds.withSizeKeepingCentre(comp_bounds.getHeight() * prefratio, comp_bounds.getHeight());
                }

                if (compw < comp_bounds.getWidth()) {
                    float ratio = static_cast<float>(comp_bounds.getWidth()) / static_cast<float>(comp_bounds.getHeight());
                    comp_bounds = comp_bounds.withSizeKeepingCentre(compw, compw / ratio);
                }

                if (valLabPos == Front) {
                    comp_bounds.translate(0, (bot - comp_bounds.getBottom()));
                }
            }

            comp_area = comp_bounds/*.translated(0, -8)*/;
            component->setBounds(comp_area);

            /*Rectangle<int> upper_margin(bounds.getTopLeft(), comp_bounds.getTopLeft());
            Rectangle<int> lower_margin(comp_bounds.getBottomRight(), bounds.getBottomRight());*/

			// define valueband as a band right beneath the component bounds, with the width of bounds and the height of textsize * 1.2f
            Rectangle<int> valueband = getLocalBounds().withHeight(textsize * 1.2f).translated(0, comp_area.getBottom());

            text_area = textband;
            //value_area = bottomband;
            value_area = valueband;

            valueLabel->setUnityOfMeasure(unityofmeasure);
            valueLabel->setDecimalPlaces(decimalplaces);

            if (auto p = dynamic_cast<BorisPtcDial*>(component); p != nullptr) {
                valueLabel->prependX();
            }

            if (c->showValLab() == Bottom) {
                valueLabel->setBounds(value_area);
            }
            else if (c->showValLab() == Front) {
                valueLabel->setBounds(comp_area);
                valueLabel->setTransmitEvents(true);
            }
        }
        else if (style == TextSliderValue) {
            Rectangle<int> textband;
            if (c->placeText() == Top) {
                auto bheight = textsize * lines * 1.2f;
                textband = bounds.removeFromTop(bheight);
            }

            Rectangle<int> bottomband;
            if (valLabPos == Bottom) {
                bottomband = bounds.removeFromBottom(STEXTSIZE * 1.2f);
            }

            Rectangle<int> comp_bounds = bounds;
            float panelratio = static_cast<float>(comp_bounds.getWidth()) / static_cast<float>(comp_bounds.getHeight());
            float prefratio = c->getPreferredRatio();

            if (prefratio) {
                auto bot = comp_bounds.getBottom();

                if (panelratio < prefratio) {
                    comp_bounds = comp_bounds.withSizeKeepingCentre(comp_bounds.getWidth(), comp_bounds.getWidth() / prefratio);
                }
                else {
                    comp_bounds = comp_bounds.withSizeKeepingCentre(comp_bounds.getHeight() * prefratio, comp_bounds.getHeight());
                }

                if (compw < comp_bounds.getWidth()) {
                    float ratio = static_cast<float>(comp_bounds.getWidth()) / static_cast<float>(comp_bounds.getHeight());
                    comp_bounds = comp_bounds.withSizeKeepingCentre(compw, compw / ratio);
                }

                if (valLabPos == Front) {
                    comp_bounds.translate(0, (bot - comp_bounds.getBottom()));
                }
            }

            comp_area = comp_bounds/*.translated(0, -8)*/;
            component->setBounds(comp_area);
            
            text_area = textband;
            value_area = bottomband;

            valueLabel->setUnityOfMeasure(unityofmeasure);
            valueLabel->setDecimalPlaces(decimalplaces);

            if (auto p = dynamic_cast<BorisPtcDial*>(component); p != nullptr) {
                valueLabel->prependX();
            }

            if (c->showValLab() == Bottom) {
                valueLabel->setBounds(value_area);
            }
        }
    }
}

void ComponentPanel::setBounds(Rectangle<int> bounds, int compw) {
    this->compw = compw;

    Component::setBounds(bounds);
}

void ComponentPanel::setBounds(Rectangle<int> bounds) {
    compw = bounds.getWidth();
    Component::setBounds(bounds);
}