#pragma once

#include <JuceHeader.h>
#include <vector>
#include "RNBO.h"
#include "CustomAudioProcessor.h"

#include "BorisGUIComp.h"
#include "BorisSlider.h"
#include "BorisToggle.h"
#include "ComponentPanel.h"
#include "BorisSubPanel.h"
#include "XYPad.h"
#include "WaveVisualiserComponent.h"
#include "DoubleBuffer.h"
#include "BorisLogo.h"
#include "BinaryData.h"

class RootComponent  :  public juce::Component,
                        public juce::Slider::Listener,
                        public juce::Button::Listener,
	                    public BorisRythmToggle::Listener,
                        public BpmListener,
	                    public stateRecallListener,
                        public juce::Timer
{
public:
    //==============================================================================
    RootComponent (std::shared_ptr<WaveVisualiserComponent> _wvc);
    ~RootComponent() override;

    //==============================================================================
    void setAudioProcessor(RNBO::JuceAudioProcessor *p);
    void updateCompForParam(unsigned long index, double value);

    void loadState();

    void resized() override;

    void sliderValueChanged (juce::Slider* sliderThatWasMoved) override;
    void buttonClicked (juce::Button* buttonThatWasClicked) override;
	void toggleChanged(int index) override;
	void bpmChanged(double newBpm) override;
	void stateRecalled() override;

    void timerCallback() override;
private:
    CustomAudioProcessor *processor = nullptr;

    Rectangle<float> waveDisplay;
    Rectangle<float> upLeftCtrlArea;
    Rectangle<float> upRightCtrlArea;
    Rectangle<float> downRightCtrlArea;


    Image notALogo;
	std::unique_ptr<BorisLogo> logo;

    BorisSplitSubPanel timePanel;
    BorisSubPanel pitchPanel;
    BorisSplitSubPanel shapePanel;
    BorisSubPanel wavectrlsPanel;
    
    std::shared_ptr<WaveVisualiserComponent> _waveVisualiser;
    XYPad xyPad;

    //==============================================================================

    struct ComponentEntry {
        std::shared_ptr<juce::Component> component;
        std::shared_ptr<ComponentPanel> panel;
    };
    std::vector<ComponentEntry> components;

    struct ComponentConfig {
        std::string type;
        std::string name;
        const char* label;
        int tsize;
        int tlines;
        std::string uom;
        int dp;
        CompPanelStyle pstyle;
    };

    // List of components with names and labels
    std::vector<ComponentConfig> componentConfigs = {
        {"BorisDenDial",            "den",  "DENSITY",          MTEXTSIZE, 3, "",    0, TextDial },               //0
        {"BorisNumberBoxSlider",    "cha",  "chance",           STEXTSIZE, 1, " %",  0, TextNumBox },               //1
        {"BorisNumberBoxSlider",    "rdl",  "rdm delay",        STEXTSIZE, 1, " %",  0, TextNumBox },               //2
        {"BorisLenDial",            "len",  "SIZE",             MTEXTSIZE, 1, " ms", 1, TextDialValue },          //3
        {"BorisNumberBoxSlider",    "rle",  "rdm size",         STEXTSIZE, 1, " %",  0, TextNumBox },               //4
        {"BorisPtcDial",            "psh",  "PITCH\nSHIFT",     MTEXTSIZE, 2, "",    2, TextDialValue},          //5
        {"BorisNumberBoxSlider",    "rpt",  "rdm shift",        STEXTSIZE, 1, " %",  0, TextNumBox },               //6
        {"BorisMoonJKnobWrapper",   "env",  "ENVELOPE",         MTEXTSIZE, 1, "",    0, TextDial },               //7
        {"BorisNumberBoxSlider",    "frp",  "reverse←",         STEXTSIZE, 1, " %",  0, LeftTextNumBox },           //8
        {"BorisInvisibleSlider",    "cpo",  "POSITION",         MTEXTSIZE, 1, "",    0, Invisible },                //9
        {"BorisDrfNumBox",          "drf",  "DRIFT",            STEXTSIZE, 1, " %",  0, LeftTextNumBox },           //10
        {"BorisNumberBoxSlider",    "pwi",  "pan width",        STEXTSIZE, 1, " %",  0, TextNumBox },               //11
        {"BorisNumberBoxSlider",    "rvo",  "rdm vol",          STEXTSIZE, 1, " %",  0, TextNumBox },               //12
        {"BorisVolumeSlider",       "gai",  "IN",               MTEXTSIZE, 1, " dB", 1, TextSliderValue },          //13
        {"BorisDial",               "fdb",  "FEEDBACK",         STEXTSIZE, 1, " %",  0, TextDialValue },          //14
        {"BorisVerticalSlider",     "wet",  "OUT",              MTEXTSIZE, 1, " dB", 1, TextSliderValue },          //15
        {"BorisToggle",             "mut",  "M",                MTEXTSIZE, 0, "",    0, RoundToggle},              //16
        {"BorisToggle",             "frz",  "\xE2\x9D\x84",     MTEXTSIZE, 0, "",    0, RoundToggle},              //17
        {"BorisToggle",             "syc",  "T sync",           MTEXTSIZE, 0, "",    0, RoundToggle },              //18
		{"BorisTmpDial",            "tmp",  "TEMPO",            MTEXTSIZE, 3, "",    0, TextDial },               //19
        {"BorisRythmToggle",        "rtm",  "Note rythm",       STEXTSIZE, 0, "",    0, TriTab}                    //20
    };

    BorisMoonJKnobWrapper* envelopeEncoderComponent = nullptr;

    static void freeBuffer(RNBO::ExternalDataId id, char *data) { free(data); }

	double bpm{ 120 }; // default fallback when host does not provide info

    void beginSyncFromProcessor();

    void recalculateMaxGrainLength();
    void setWaveDisplayState(bool freezed);
    void setSyncMode(bool sync);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RootComponent)
};