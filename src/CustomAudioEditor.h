#pragma once

#include "JuceHeader.h"
#include "RNBO.h"
// #include "CustomAudioProcessor.h"
#include "RNBO_JuceAudioProcessor.h"
#include "RootComponent.h"
// #include "melatonin_inspector/melatonin_inspector.h"

#define INSPECTOR 0

class CustomAudioEditor : public AudioProcessorEditor, private AudioProcessorListener
{
public:
    //CustomAudioEditor(RNBO::JuceAudioProcessor* const p, RNBO::CoreObject& rnboObject);
    CustomAudioEditor(RNBO::JuceAudioProcessor* const p, RNBO::CoreObject& rnboObject, std::shared_ptr<WaveVisualiserComponent> wvc);
    ~CustomAudioEditor() override;

private:
    void audioProcessorChanged (AudioProcessor*, const ChangeDetails&) override {}
    void audioProcessorParameterChanged(AudioProcessor*, int parameterIndex, float) override;

protected:
	std::shared_ptr<WaveVisualiserComponent>    _waveVisualiserComponent;
	std::unique_ptr<RootComponent>              _rootComponent;

    RNBO::JuceAudioProcessor                    *_audioProcessor;
    RNBO::CoreObject&                           _rnboObject; 

    #if INSPECTOR
    melatonin::Inspector inspector { *this, false };
    #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomAudioEditor)
};