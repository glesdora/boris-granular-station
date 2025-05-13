#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"
#include <json/json.hpp>

#ifdef RNBO_INCLUDE_DESCRIPTION_FILE
#include <rnbo_description.h>
#endif

CustomAudioProcessor* CustomAudioProcessor::CreateDefault() {
	nlohmann::json patcher_desc, presets;

#ifdef RNBO_BINARY_DATA_STORAGE_NAME
	extern RNBO::BinaryDataImpl::Storage RNBO_BINARY_DATA_STORAGE_NAME;
	RNBO::BinaryDataImpl::Storage dataStorage = RNBO_BINARY_DATA_STORAGE_NAME;
#else
	RNBO::BinaryDataImpl::Storage dataStorage;
#endif
	RNBO::BinaryDataImpl data(dataStorage);

#ifdef RNBO_INCLUDE_DESCRIPTION_FILE
	patcher_desc = RNBO::patcher_description;
	presets = RNBO::patcher_presets;
#endif
  return new CustomAudioProcessor(patcher_desc, presets, data);
}

CustomAudioProcessor::CustomAudioProcessor(
    const nlohmann::json& patcher_desc,
    const nlohmann::json& presets,
    const RNBO::BinaryData& data
    ) 
  : RNBO::JuceAudioProcessor(patcher_desc, presets, data)
{
    std::unique_ptr<InputStream> stream(new MemoryInputStream(BinaryData::envelopes_table, BinaryData::envelopes_table_Size, false));
    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(std::move(stream)));

    env_shapes.setSize(reader->numChannels, reader->lengthInSamples);
    reader->read(&env_shapes, 0, reader->lengthInSamples, 0, true, true);

    envelopeDB = new DoubleBuffer(env_shapes.getNumSamples() / 3);

    EnvelopeTable et;
    et.data = env_shapes.getArrayOfReadPointers()[0];
    et.envsize = env_shapes.getNumSamples() / 3;
    et.numberOfShapes = 3;
    et.peaks = {8, 36, 92};

    envint.reset(new EnvelopesInterpolator(et));

    auto& inactiveBuffer = envelopeDB->getInactiveBuffer();
    envint->interpolate(1, inactiveBuffer);
    envelopeDB->swapBuffers();

    _waveVisualiser = std::make_shared<WaveVisualiserComponent>(1);
}

CustomAudioProcessor::~CustomAudioProcessor()
{
	if (vectorToDisplay) {
		free(vectorToDisplay);
	}
	if (envelopeBuffer) {
		free(envelopeBuffer);
	}
	delete envelopeDB;
}

AudioProcessorEditor* CustomAudioProcessor::createEditor()
{
    return new CustomAudioEditor(this, this->_rnboObject, _waveVisualiser);
}

void CustomAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    RNBO::CoreObject& coreObject = this->getRnboObject();

    RNBO::Float32AudioBuffer bufferType(1, sampleRate);

    const uint32_t sampleBufferSize = sizeof(float) * samplesPerBlock;
    vectorToDisplay = (float *) malloc(sampleBufferSize);
    coreObject.setExternalData(
        "inter_databuf_01",
        reinterpret_cast<char *>(vectorToDisplay),
        samplesPerBlock * sizeof(float) / sizeof(char),
        bufferType,
        &freeBuffer
    );

     const uint32_t envelopeBufferSize = sizeof(float) * env_shapes.getNumSamples() / 3;
     coreObject.setExternalData(
         "interpolated_envelope",
         reinterpret_cast<char *>(envelopeDB->getActiveBuffer().data()),
         envelopeBufferSize / sizeof(char),
         bufferType,
         &freeBuffer
     );

	_waveVisualiser->prepareToDisplay(sampleRate, samplesPerBlock);

    RNBO::JuceAudioProcessor::prepareToPlay(sampleRate, samplesPerBlock);
}

void CustomAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	if (auto posInfo = getPlayHead()->getPosition()) {
		if (auto bpmFromHost = posInfo->getBpm())
            if (bpmFromHost != bpm) {
                bpm = bpmFromHost;
                notifyBpmListeners(*bpmFromHost);
            }
	}

	auto blocksize = buffer.getNumSamples();

    RNBO::JuceAudioProcessor::processBlock(buffer, midiMessages);
    _waveVisualiser->pushBlock(vectorToDisplay, blocksize);
}

void CustomAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
     //Convert inputs from double to float
    auto numChannels = buffer.getNumChannels();
    auto n = buffer.getNumSamples();

    auto input_buf = buffer.getArrayOfReadPointers();
    std::vector<std::vector<float>> floatInputs(numChannels);
    std::vector<const float*> floatInputPtrs(numChannels);
    
    for (int i = 0; i < numChannels; ++i) {
        for (int j = 0; j < n; ++j) {
            floatInputs[i][j] = static_cast<float>(input_buf[i][j]);
        }
        floatInputPtrs[i] = floatInputs[i].data();
    }

    RNBO::JuceAudioProcessor::processBlock(buffer, midiMessages);

}

void CustomAudioProcessor::interpolate(float s)
{
     auto& inactiveBuffer = envelopeDB->getInactiveBuffer();
	 envint->interpolate(s, inactiveBuffer);
     envelopeDB->swapBuffers();
}

void CustomAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	JuceAudioProcessor::setStateInformation(data, sizeInBytes);
	stateRestored = true;
	notifyStateRecallListeners();
}

bool CustomAudioProcessor::hasRestoredState() const
{
	return stateRestored;
}