#include "CustomAudioEditor.h"

CustomAudioEditor::CustomAudioEditor(RNBO::JuceAudioProcessor* const p, RNBO::CoreObject& rnboObject, std::shared_ptr<WaveVisualiserComponent> wvc)
    : AudioProcessorEditor(p)
    , _rnboObject(rnboObject)
    , _audioProcessor(p)
    , _waveVisualiserComponent(wvc)
{
    _audioProcessor->AudioProcessor::addListener(this);

    _rootComponent.reset(new RootComponent(_waveVisualiserComponent)); // initialise the root component

    _rootComponent->setAudioProcessor(p);
    addAndMakeVisible(_rootComponent.get());
    setSize(_rootComponent->getWidth(), _rootComponent->getHeight());
}

CustomAudioEditor::~CustomAudioEditor()
{
    _audioProcessor->AudioProcessor::removeListener(this);
}

void CustomAudioEditor::audioProcessorParameterChanged(AudioProcessor*, int parameterIndex, float value)
{
    _rootComponent->updateCompForParam(parameterIndex, value);
}