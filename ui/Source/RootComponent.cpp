#include "RootComponent.h"

RootComponent::RootComponent(std::shared_ptr<WaveVisualiserComponent> _wvc) : timePanel(borisPalette[border].withAlpha(0.5f)),
pitchPanel(borisPalette[border].withAlpha(0.5f)),
shapePanel(borisPalette[border].withAlpha(0.5f), BorisSubPanel::SubPanelShape::Rounded, 0.8f, true),
wavectrlsPanel(borisPalette[inactive].withAlpha(0.2f), BorisSubPanel::SubPanelShape::Plain)
{

    notALogo = juce::ImageCache::getFromMemory(BinaryData::birds_png, BinaryData::birds_png_Size);

    std::unique_ptr<XmlElement> snowflakeXml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::snowflake_svg), BinaryData::snowflake_svg_Size)));
    std::unique_ptr<XmlElement> muteXml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::mute_svg), BinaryData::mute_svg_Size)));
    std::unique_ptr<XmlElement> metronomeXml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::metronome_svg), BinaryData::metronome_svg_Size)));
    std::unique_ptr<XmlElement> normalNoteXml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::normalnote_svg), BinaryData::normalnote_svg_Size)));
    std::unique_ptr<XmlElement> dottedNoteXml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::dottednote_svg), BinaryData::dottednote_svg_Size)));
    std::unique_ptr<XmlElement> tripletNoteXml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::tripletnote_svg), BinaryData::tripletnote_svg_Size)));
    std::unique_ptr<XmlElement> rythm64Xml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::rythm64_svg), BinaryData::rythm64_svg_Size)));
    std::unique_ptr<XmlElement> rythm32Xml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::rythm32_svg), BinaryData::rythm32_svg_Size)));
    std::unique_ptr<XmlElement> rythm16Xml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::rythm16_svg), BinaryData::rythm16_svg_Size)));
    std::unique_ptr<XmlElement> rythm8Xml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::rythm8_svg), BinaryData::rythm8_svg_Size)));
    std::unique_ptr<XmlElement> rythm4Xml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::rythm4_svg), BinaryData::rythm4_svg_Size)));
    std::unique_ptr<XmlElement> rythm2Xml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::rythm2_svg), BinaryData::rythm2_svg_Size)));
    std::unique_ptr<XmlElement> rythm1Xml(XmlDocument::parse(String::fromUTF8(reinterpret_cast<const char*>(BinaryData::rythm1_svg), BinaryData::rythm1_svg_Size)));

    // Loop to create each component based on config
    for (const auto& config : componentConfigs)
    {
        // Create the component based on type
        std::shared_ptr<juce::Component> comp;
        if (config.type == "BorisToggle")
            comp = std::make_shared<BorisToggle>(config.name);
        else if (config.type == "BorisDial")
            comp = std::make_shared<BorisDial>(config.name);
        else if (config.type == "BorisDenDial")
            comp = std::make_shared<BorisDenDial>(config.name);
        else if (config.type == "BorisLenDial")
            comp = std::make_shared<BorisLenDial>(config.name);
        else if (config.type == "BorisFdbDial")
            comp = std::make_shared<BorisFdbDial>(config.name);
        else if (config.type == "BorisPtcDial")
            comp = std::make_shared<BorisPtcDial>(config.name);
        else if (config.type == "BorisHorizontalSlider")
            comp = std::make_shared<BorisHorizontalSlider>(config.name);
        else if (config.type == "BorisVerticalSlider")
            comp = std::make_shared<BorisVerticalSlider>(config.name);
        else if (config.type == "BorisVolumeSlider")
            comp = std::make_shared<BorisVolumeSlider>(config.name);
        else if (config.type == "BorisInvisibleSlider")
            comp = std::make_shared<BorisInvisibleSlider>(config.name);
        else if (config.type == "BorisMoonJKnobWrapper")
            comp = std::make_shared<BorisMoonJKnobWrapper>(config.name);
        else if (config.type == "BorisNumberBoxSlider")
            comp = std::make_shared<BorisNumberBoxSlider>(config.name);
        else if (config.type == "BorisDrfNumBox")
            comp = std::make_shared<BorisDrfNumBox>(config.name);
        else if (config.type == "BorisRythmToggle")
            comp = std::make_shared<BorisRythmToggle>(config.name, 3);
        else if (config.type == "BorisTmpDial")
            comp = std::make_shared<BorisTmpDial>(config.name, 7);

        // Customize component
        if (auto* slider = dynamic_cast<juce::Slider*>(comp.get())) {
            slider->addListener(this);

            if (auto* tslider = dynamic_cast<BorisTmpDial*>(comp.get())) {
                std::vector<const XmlElement*> icons;
                icons.push_back(rythm16Xml.get());
                icons.push_back(rythm32Xml.get());
                icons.push_back(rythm16Xml.get());
                icons.push_back(rythm8Xml.get());
                icons.push_back(rythm4Xml.get());
                icons.push_back(rythm2Xml.get());
                icons.push_back(rythm1Xml.get());
                tslider->loadIcons(icons);
            }
        }
        else if (auto* button = dynamic_cast<BorisToggle*>(comp.get())) {
            button->setTriggeredOnMouseDown(true);
            //button->setToggleState(true, juce::dontSendNotification);
            button->addListener(this);

            if (button->getName() == "mut") {
                button->loadIcon(*muteXml);
                button->setColours(borisPalette[mute], borisPalette[active]);
            }
            else if (button->getName() == "frz") {
                button->loadIcon(*snowflakeXml);
                button->setColours(borisPalette[freeze], borisPalette[active]);
            }
            else if (button->getName() == "syc") {
                button->loadIcon(*metronomeXml);
                button->setColours(borisPalette[active], borisPalette[mute]);
            }
        }
        else if (auto* multiToggle = dynamic_cast<BorisRythmToggle*>(comp.get())) {
            multiToggle->addListener(this);

            std::vector<const XmlElement*> icons;
            icons.push_back(normalNoteXml.get());
            icons.push_back(dottedNoteXml.get());
            icons.push_back(tripletNoteXml.get());

            multiToggle->loadIcons(icons);
        }

        // Create the panel and add to the component list
        if (auto* bcomp = dynamic_cast<BorisGUIComp*>(comp.get())) {
            auto panel = std::make_shared<ComponentPanel>(comp.get(), config.label, config.tsize, config.tlines, config.uom, config.dp, config.pstyle);
            addAndMakeVisible(panel.get());
            components.push_back({ comp, panel }); // Store the component and panel in the components vector
        }
    }

    addAndMakeVisible(xyPad);
    xyPad.setColours(borisPalette[contrast], borisPalette[border]);
    xyPad.setBufferedToImage(true);

    auto position = std::dynamic_pointer_cast<Slider>(components[9].component).get();
    auto drift = std::dynamic_pointer_cast<Slider>(components[10].component).get();
    xyPad.registerSlider(position, XYPad::Axis::X);
    xyPad.registerSlider(drift, XYPad::Axis::Y);

    _waveVisualiser = _wvc;
    _waveVisualiser->setPalette(borisPalette[back], borisPalette[led], borisPalette[label]);
    addAndMakeVisible(_waveVisualiser.get());

    envelopeEncoderComponent = dynamic_cast<BorisMoonJKnobWrapper*>(components[7].component.get());
    envelopeEncoderComponent->setNumberOfShapes(3);

    logo.reset(new BorisLogo(notALogo));
    addAndMakeVisible(logo.get());

    addAndMakeVisible(timePanel);
    addAndMakeVisible(pitchPanel);
    addAndMakeVisible(shapePanel);
    addAndMakeVisible(wavectrlsPanel);


    setSize(630, 420);
}

RootComponent::~RootComponent()
{
    auto position = std::dynamic_pointer_cast<Slider>(components[9].component).get();
    auto drift = std::dynamic_pointer_cast<Slider>(components[10].component).get();
    xyPad.deregisterSlider(position);
    xyPad.deregisterSlider(drift);

    if (processor == nullptr) return;
    processor->removeBpmListener(this);
    processor->removeStateRecallListener(this);
}

void RootComponent::resized()
{
    Rectangle<int> bounds = getLocalBounds().reduced(2);
    auto w = bounds.getWidth();
    auto h = bounds.getHeight();

    Rectangle<float> fbounds = bounds.toFloat();

    Point<float> croix = { w * 0.75f, h * 0.475f };     // Split the whole area in 4 parts

    waveDisplay = Rectangle<float>(fbounds.getX(), fbounds.getY() + croix.y, croix.x, h - croix.y);
    upLeftCtrlArea = Rectangle<float>(fbounds.getX(), fbounds.getY(), croix.x, croix.y);
    upRightCtrlArea = Rectangle<float>(fbounds.getX() + croix.x, fbounds.getY(), w - croix.x, croix.y);
    downRightCtrlArea = Rectangle<float>(fbounds.getX() + croix.x, fbounds.getY() + croix.y, w - croix.x, h - croix.y);

    float up_h = upLeftCtrlArea.getHeight();
    //float lo_h = downRightCtrlArea.getHeight();
    float upleftbasewidth = upLeftCtrlArea.getWidth() * 0.2f;

    Rectangle<float> ulArea1 = Rectangle<float>(upLeftCtrlArea.getX(), upLeftCtrlArea.getY(), upleftbasewidth * 2, up_h);
    Rectangle<float> logoArea = Rectangle<float>(upLeftCtrlArea.getX() + ulArea1.getWidth(), upLeftCtrlArea.getY() + up_h * 0.166f, upleftbasewidth * 2, up_h * 0.66f);
    Rectangle<float> ulArea2 = Rectangle<float>(upLeftCtrlArea.getX() + ulArea1.getWidth() + logoArea.getWidth(), upLeftCtrlArea.getY(), upleftbasewidth, up_h);
    Rectangle<float> urArea = upRightCtrlArea;

    Rectangle<float> drArea1;
    Rectangle<float> drArea2;
    Rectangle<float> drArea3;
    {
        Rectangle<float> dr = downRightCtrlArea;
        drArea3 = dr.removeFromRight(dr.getWidth() * 0.33f);
        drArea1 = dr.removeFromTop(dr.getHeight() * 0.5f);
        drArea2 = dr;
    }

    ulArea1 = ulArea1.reduced(5);
    timePanel.setBounds(ulArea1.toNearestInt());
    timePanel.toBack();
    ulArea2 = ulArea2.reduced(5);
    pitchPanel.setBounds(ulArea2.toNearestInt());
    pitchPanel.toBack();
    urArea = urArea.reduced(5);
    shapePanel.setBounds(urArea.toNearestInt());
    shapePanel.toBack();

    // Upper left area

    int numboxwidth = static_cast<int>(ulArea1.getWidth() * 0.33f);
    int buttonwidth = static_cast<int>(numboxwidth * 0.45f);
    int sliderwidth = static_cast<int>(numboxwidth * 0.4f);
    int dialwidth = static_cast<int>(numboxwidth * 1.33f);

    //int denpanelheight = dialwidth / 1.21f + MTEXTSIZE * 1.5f;
    float lenpanelheight = dialwidth / 1.21f + MTEXTSIZE * 1.5f + STEXTSIZE * 1.2f;

    // Area 1: "squarish" area left of the logo

    Rectangle<float> ul_a1_left = ulArea1.removeFromLeft(ulArea1.getWidth() * 0.5f).reduced(3);
    Rectangle<float> ul_a1_right = ulArea1.reduced(3);
    Rectangle<float> ul_a1_BL = ul_a1_left.removeFromBottom(MTEXTSIZE * 3.0f);
    Rectangle<float> ul_a1_TL = ul_a1_left;
    //Rectangle<float> ul_a1_TL2 = ul_a1_TL/*.withTrimmedBottom(MTEXTSIZE * 0.5f)*/;
    //Rectangle<float> ul_a1_TL2 = ul_a1_left.removeFromTop(lenpanelheight);
    Rectangle<float> ul_a1_TL2 = ul_a1_TL;
    //Rectangle<float> ul_a1_BL = ul_a1_left;
    Rectangle<float> ul_a1_TR = ul_a1_right.removeFromTop(lenpanelheight);
    Rectangle<float> ul_a1_BR = ul_a1_right;

    components[19].panel->setVisible(false);
    components[20].panel->setVisible(false);

    components[0].panel->setBounds(ul_a1_TL2.toNearestInt(), dialwidth);    // DEN

    components[19].panel->setBounds(ul_a1_TL2.toNearestInt(), dialwidth);	   // TMP

    auto tritogheight = ul_a1_TL2.getHeight() * 0.15f;
    components[20].panel->setBounds(ul_a1_TL2.removeFromBottom(tritogheight).translated(0, tritogheight * 0.75f).toNearestInt(), numboxwidth);    // RYTHM

    components[18].panel->setBounds(ul_a1_TL2.removeFromTop(static_cast<float>(buttonwidth)).translated(0, MTEXTSIZE * 1.4f).toNearestInt(), buttonwidth);	// SYNC

    components[18].panel->toFront(true);

    components[3].panel->setBounds(ul_a1_TR.toNearestInt(), dialwidth);    // LEN

    auto rdl_bounds = ul_a1_BR.removeFromBottom(ul_a1_BR.getHeight() * 0.5f).withTrimmedBottom(3);
    components[2].panel->setBounds(rdl_bounds.toNearestInt(), numboxwidth);   // RDL

    auto rle_bounds = ul_a1_BR;
    components[4].panel->setBounds(rle_bounds.toNearestInt(), numboxwidth);    // RLE

    auto cha_bounds = ul_a1_BL.withTrimmedBottom(3);
    components[1].panel->setBounds(cha_bounds.toNearestInt(), numboxwidth);    // CHA

    // logo

    logo->setBounds(logoArea.toNearestInt());
    //components[18].panel->setBounds(logoArea.removeFromTop(logoArea.getHeight()*0.5f).toNearestInt(), buttonwidth);    // LOGO

    // Area 2: right of the logo
    ulArea2 = ulArea2.reduced(3);
    Rectangle<float> ul_a2_top = ulArea2.removeFromTop(ulArea2.getHeight() * 0.75f);
    Rectangle<float> ul_a2_bottom = ulArea2;

    components[5].panel->setBounds(ul_a2_top.toNearestInt());    // CPT

    auto rpt_bounds = ul_a2_bottom.withTrimmedBottom(3);
    components[6].panel->setBounds(rpt_bounds.toNearestInt(), numboxwidth);    // RPT

    // Upper right area
    float tr_h_prop = 0.8f;

    urArea = urArea.reduced(3);
    Rectangle<float> ur_top = urArea.removeFromTop(urArea.getHeight() * tr_h_prop);
    Rectangle<float> ur_bottom = urArea;

    auto rev_bounds = ur_bottom.withSizeKeepingCentre(numboxwidth * 2.0f, ur_bottom.getHeight());

    components[7].panel->setBounds(ur_top.toNearestInt());    // ENV
    components[8].panel->setBounds(rev_bounds.reduced(2).toNearestInt(), numboxwidth);    // FRP

    // Wave display
    Rectangle<float> wavedispreduced = waveDisplay.reduced(2);
    Rectangle<float> wavectrlslayer(wavedispreduced);
    Rectangle<float> wavectrlsarea = wavectrlslayer.removeFromTop(buttonwidth * 1.1f);
    Rectangle<float> xypadarea = wavectrlslayer;
    xyPad.setBounds(xypadarea.toNearestInt());
    wavectrlsPanel.setBounds(wavectrlsarea.toNearestInt());
    wavectrlsPanel.toBack();

    if (_waveVisualiser != nullptr) {
        Rectangle<float> waveBounds = wavedispreduced.withTrimmedBottom(2.0f).withTrimmedRight(2.0f).withTrimmedTop(2.0f);      // like a reduced(2.0f), but I keep left intact
        _waveVisualiser->setBounds(waveBounds.toNearestInt());
        _waveVisualiser->toBack();
    }

    // wavectrlsarea = wavectrlsarea.reduced(wavectrlsPanel.getReductionDelta()*1.5f);        
    Rectangle<float> freezeBounds = wavectrlsarea.withTrimmedLeft(wavectrlsarea.getWidth() - buttonwidth * 1.1f);
    components[17].panel->setBounds(freezeBounds.toNearestInt(), buttonwidth);    // REC
    Rectangle<float> driftBounds = wavectrlsarea.withSizeKeepingCentre(numboxwidth * 2.0f, wavectrlsarea.getHeight());
    components[10].panel->setBounds(driftBounds.toNearestInt(), numboxwidth);    // SPR

    // Down right area (no sub panels)
    Rectangle<float> reduced_dr = downRightCtrlArea.reduced(5);
    auto wet_bounds = reduced_dr.removeFromRight(buttonwidth * 1.5f);

    components[15].panel->setBounds(wet_bounds.reduced(2).toNearestInt(), sliderwidth);    // WET

    auto gain_bounds = reduced_dr.removeFromLeft(buttonwidth * 1.5f);

    auto feedback_bounds = reduced_dr.removeFromTop(reduced_dr.getHeight() * 0.5f);
    feedback_bounds = feedback_bounds.withSizeKeepingCentre(feedback_bounds.getWidth(), feedback_bounds.getHeight() * 0.8f);

    auto mute_bounds = reduced_dr.removeFromTop(buttonwidth * 1.1f);

    reduced_dr = reduced_dr.removeFromTop(reduced_dr.getHeight() * 0.85f);
    auto pan_bounds = reduced_dr.removeFromTop(reduced_dr.getHeight() * 0.5f);
    auto rdmvol_bounds = reduced_dr;

    components[14].panel->setBounds(feedback_bounds.toNearestInt(), static_cast<int>(dialwidth * 0.75f));     // FDB
    components[16].panel->setBounds(mute_bounds.toNearestInt(), buttonwidth);               // PLY
    components[12].panel->setBounds(rdmvol_bounds.reduced(2).toNearestInt(), numboxwidth);             // MIN
    components[11].panel->setBounds(pan_bounds.reduced(2).toNearestInt(), numboxwidth);                // PWI
    components[13].panel->setBounds(gain_bounds.reduced(2).toNearestInt(), sliderwidth);               // GAI
}

void RootComponent::sliderValueChanged(juce::Slider* sliderThatWasMoved)
{
    if (processor == nullptr) return;
    RNBO::CoreObject& coreObject = processor->getRnboObject();
    auto parameters = processor->getParameters();
    auto movedSliderValue = sliderThatWasMoved->getValue();

    // check the sliders handling logic
    // maybe i want cast before and store in a list, but the indexes have to be consistent (my order, the rnbo order and the slider list)

    if (sliderThatWasMoved == dynamic_cast<Slider*>(components[7].component.get())) {
        processor->interpolate(static_cast<float>(movedSliderValue));
        envelopeEncoderComponent->updateShapePath();
    }
    else if (sliderThatWasMoved == dynamic_cast<Slider*>(components[19].component.get())) {
        juce::MessageManager::callAsync([this] {
            recalculateMaxGrainLength();
            });
    }

    int index = coreObject.getParameterIndexForID(sliderThatWasMoved->getName().toRawUTF8());
    if (index != -1) {
        const auto param = processor->getParameters()[index];
        auto newVal = movedSliderValue;
        float normalizedValue = static_cast<float>(coreObject.convertToNormalizedParameterValue(index, newVal));
        if (param && param->getValue() != normalizedValue)
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(normalizedValue);
            param->endChangeGesture();
        }
    }
}

void RootComponent::buttonClicked(juce::Button* buttonThatWasClicked)
{
    if (processor == nullptr) return;
    RNBO::CoreObject& coreObject = processor->getRnboObject();

    //auto button_mute = std::dynamic_pointer_cast<Button>(components[16].component).get();
    auto button_freeze = std::dynamic_pointer_cast<Button>(components[17].component).get();
    auto button_sync = std::dynamic_pointer_cast<Button>(components[18].component).get();

    auto togglestate = buttonThatWasClicked->getToggleState();

    if (buttonThatWasClicked == button_freeze)
    {
        this->setWaveDisplayState(togglestate);
    }
    else if (buttonThatWasClicked == button_sync) {
        this->setSyncMode(togglestate);
    }

    int index = coreObject.getParameterIndexForID(buttonThatWasClicked->getName().toRawUTF8());

    if (index != -1) {
        const auto param = processor->getParameters()[index];
        auto newVal = buttonThatWasClicked->getToggleState();

        if (param && static_cast<bool>(param->getValue()) != newVal)
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(newVal);
            param->endChangeGesture();
        }
    }
}

void RootComponent::toggleChanged(int index) {
    if (processor == nullptr) return;
    RNBO::CoreObject& coreObject = processor->getRnboObject();
    auto parameters = processor->getParameters();

    auto param = processor->getParameters()[20];
    auto normalizedValue = static_cast<float>(coreObject.convertToNormalizedParameterValue(20, index));

    if (param && param->getValue() != normalizedValue) {
        param->beginChangeGesture();
        param->setValueNotifyingHost(normalizedValue);
        param->endChangeGesture();
    }
}

void RootComponent::setAudioProcessor(RNBO::JuceAudioProcessor* p)
{
    processor = dynamic_cast<CustomAudioProcessor*>(p);
    if (processor == nullptr) return;

    RNBO::ParameterInfo parameterInfo;
    RNBO::CoreObject& coreObject = processor->getRnboObject();

    processor->addBpmListener(this);
    processor->addStateRecallListener(this);

    auto& envelopeDB = processor->getEnvelopeDBRef();
    envelopeEncoderComponent->setData(envelopeDB);

    auto sr = processor->getSampleRate();
    auto bs = processor->getBlockSize();
    if (sr > 0 && bs > 0) {
        _waveVisualiser->prepareToDisplay(sr, bs);
    }

    for (unsigned long i = 0; i < coreObject.getNumParameters(); i++) {
        auto parameterName = coreObject.getParameterId(i);
        //RNBO::ParameterValue value = coreObject.getParameterValue(i);

        Slider* slider = nullptr;
        Button* button = nullptr;
        BorisRythmToggle* borisryt = nullptr;

        auto c = components[i].component;
        if (c->getName() != juce::String(parameterName))
            jassertfalse;
        else {
            slider = std::dynamic_pointer_cast<Slider>(c).get();
            button = std::dynamic_pointer_cast<Button>(c).get();
            borisryt = std::dynamic_pointer_cast<BorisRythmToggle>(c).get();
        }

        if (slider) {
            coreObject.getParameterInfo(i, &parameterInfo);

            BorisSlider* borisSlider = dynamic_cast<BorisSlider*>(slider);

            if (borisSlider == nullptr) {
                float interval = 0.0f;
                if (auto steps = parameterInfo.steps; steps > 0) {
                    interval = static_cast<float>(parameterInfo.max - parameterInfo.min) / static_cast<float>(steps);
                }
                slider->setRange(parameterInfo.min, parameterInfo.max, interval);     //set the range of the slider                
            }
            else {
                borisSlider->setRange(parameterInfo.min, parameterInfo.max);     //set the range of the slider
            }

            auto defaultValue = parameterInfo.initialValue;
            if (auto dial = dynamic_cast<BorisDial*>(slider); dial != nullptr) {
                dial->setDoubleClickReturnValue(true, defaultValue);
            }
            else if (auto numbox = dynamic_cast<BorisNumberBoxSlider*>(slider); numbox != nullptr) {
                numbox->setDoubleClickReturnValue(true, defaultValue);
            }
        }
    }

    this->stateRecalled();
}

void RootComponent::timerCallback()
{
    stopTimer();

    this->loadState();
}

void RootComponent::beginSyncFromProcessor()
{
    if (processor == nullptr) return;

    startTimer(30);
}

void RootComponent::updateCompForParam(unsigned long index, double value)
{
    if (processor == nullptr) return;
    RNBO::CoreObject& coreObject = processor->getRnboObject();
    auto denormalizedValue = coreObject.convertFromNormalizedParameterValue(index, value);

    auto slider = dynamic_cast<Slider*>(components[(int)index].component.get());
    auto button = dynamic_cast<Button*>(components[(int)index].component.get());
    auto borisryt = dynamic_cast<BorisRythmToggle*>(components[index].component.get());

    if (slider && (slider->getThumbBeingDragged() == -1)) {
        juce::MessageManager::callAsync([=]() {
            slider->setValue(denormalizedValue, NotificationType::sendNotification);
            });

        if (auto* drfnumbox = dynamic_cast<BorisDrfNumBox*>(slider); drfnumbox != nullptr) {
            if (!xyPad.getControlledByMouse()) {
                auto min = slider->getMinimum();
                auto max = slider->getMaximum();
                juce::MessageManager::callAsync([this, denormalizedValue, min, max] {
                    xyPad.updateThumbPosition(XYPad::Y, denormalizedValue, min, max);
                    });
            }
        }
        else if (auto* posslider = dynamic_cast<BorisInvisibleSlider*>(slider); posslider != nullptr) {
            if (!xyPad.getControlledByMouse()) {
                auto min = slider->getMinimum();
                auto max = slider->getMaximum();
                juce::MessageManager::callAsync([this, denormalizedValue, min, max] {
                    xyPad.updateThumbPosition(XYPad::X, denormalizedValue, min, max);
                    });
            }
        }
    }
    else if (button) {
        juce::MessageManager::callAsync([=]() {
            button->setToggleState(static_cast<bool>(denormalizedValue), NotificationType::sendNotification);

            });
    }
    else if (borisryt) {
        borisryt->setActiveToggle(static_cast<int>(denormalizedValue));
    }
}

void RootComponent::bpmChanged(double)
{
    // Here I know the new bpm, but I don't use it as argument. Fix the logic.
    juce::MessageManager::callAsync([this] {
        recalculateMaxGrainLength();
        });
}

void RootComponent::stateRecalled()
{
    this->loadState();
}

void RootComponent::loadState() {
    if (processor == nullptr) return;

    RNBO::CoreObject& coreObject = processor->getRnboObject();

    for (int i = 0; i < components.size(); i++) {
        String parameterName = coreObject.getParameterId(i);
        RNBO::ParameterValue value = coreObject.getParameterValue(i);

        Slider* slider = nullptr;
        Button* button = nullptr;
        BorisRythmToggle* borisryt = nullptr;

        auto c = components[i].component;
        if (c->getName() == parameterName) {
            slider = std::dynamic_pointer_cast<Slider>(c).get();
            button = std::dynamic_pointer_cast<Button>(c).get();
            borisryt = std::dynamic_pointer_cast<BorisRythmToggle>(c).get();
        }

        if (slider) {
            slider->setValue(value, juce::sendNotification);

            if (parameterName == "env") {
                envelopeEncoderComponent->updateShapePath();            //definetly weird solution
            }
            else if (parameterName == "cpo") {
                xyPad.initX(value);
            }
            else if (parameterName == "drf") {
                xyPad.initY(value);
            }
        }
        else if (button) {
            button->setToggleState(static_cast<bool>(value), juce::sendNotification);
        }
        else if (borisryt) {
            borisryt->setActiveToggle(static_cast<int>(value));
        }
        else {
            jassertfalse;
        }
    }
}

void RootComponent::recalculateMaxGrainLength() {
    auto tmpdial = dynamic_cast<BorisTmpDial*>(components[19].component.get());

    if (tmpdial == nullptr) return;

    int tmp_value = static_cast<int>(tmpdial->getValue());
    int subd = 1 << (6 - tmp_value);
    auto p_bpm = processor->getBPM();
    if (p_bpm.hasValue()) {
        double freq = *p_bpm / 240.0 * subd;

        if (auto* lendial = dynamic_cast<BorisLenDial*>(components[3].component.get()); lendial != nullptr) {
            lendial->recalculateMaxValue(freq);
        }
    }
}

void RootComponent::setWaveDisplayState(bool freezed)
{
    if (freezed)
        _waveVisualiser->freeze();
    else
        _waveVisualiser->scroll();
}

void RootComponent::setSyncMode(bool sync)
{
    auto lendial = dynamic_cast<BorisLenDial*>(components[3].component.get());
    if (lendial != nullptr) {
        juce::MessageManager::callAsync([this, lendial, sync] {
            lendial->setTempoMode(sync);
            });
    }

    if (sync) {
        components[0].panel->setVisible(false);
        components[19].panel->setVisible(true);
        components[20].panel->setVisible(true);
    }
    else {
        components[19].panel->setVisible(false);
        components[20].panel->setVisible(false);
        components[0].panel->setVisible(true);
    }
}