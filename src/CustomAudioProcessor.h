#pragma once

#include "RNBO.h"
#include "RNBO_Utils.h"
#include "RNBO_JuceAudioProcessor.h"
#include "RNBO_BinaryData.h"
#include <json/json.hpp>
 #include "WaveVisualiserComponent.h"
#include "EnvelopesInterpolator.h"
 #include "DoubleBuffer.h"
#include "BinaryData.h"

class BpmListener {
public:
    virtual ~BpmListener() = default;
    virtual void bpmChanged(double newBpm) = 0;
};

class stateRecallListener
{
public:
    virtual ~stateRecallListener() {}
    virtual void stateRecalled() = 0;
};

class CustomAudioProcessor : public RNBO::JuceAudioProcessor {
public:
    static CustomAudioProcessor* CreateDefault();
    CustomAudioProcessor(const nlohmann::json& patcher_desc, const nlohmann::json& presets, const RNBO::BinaryData& data);
    ~CustomAudioProcessor();
    juce::AudioProcessorEditor* createEditor() override;
    
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

	void setStateInformation(const void* data, int sizeInBytes) override;
    bool hasRestoredState() const;

	Optional<double> getBPM() const { return bpm; }

    void addBpmListener(BpmListener* listener) { bpmListeners.add(listener); }
    void removeBpmListener(BpmListener* listener) { bpmListeners.remove(listener); }

	void addStateRecallListener(stateRecallListener* listener) { stateRecallListeners.add(listener); }
	void removeStateRecallListener(stateRecallListener* listener) { stateRecallListeners.remove(listener); }

    void interpolate(float s);

    DoubleBuffer& getEnvelopeDBRef() { return *envelopeDB; }

private:
    float *vectorToDisplay = nullptr;
    float *envelopeBuffer = nullptr;
    static void freeBuffer(RNBO::ExternalDataId id, char *data) { free(data); }

    juce::ListenerList<BpmListener> bpmListeners;
    Optional<double> bpm = 120.0f; // Default to 120

    void notifyBpmListeners(double newBpm) {
        bpmListeners.call([newBpm](BpmListener& listener) { listener.bpmChanged(newBpm); });
    }

	ListenerList<stateRecallListener> stateRecallListeners;

	void notifyStateRecallListeners() {
		stateRecallListeners.call([](stateRecallListener& listener) { listener.stateRecalled(); });
	}

    File getPluginDirectory()
    {
#if JUCE_MAC
        return File::getSpecialLocation(File::currentExecutableFile);
#elif JUCE_WINDOWS
        return File::getSpecialLocation(File::currentApplicationFile).getParentDirectory();
#endif
    }

    std::shared_ptr<WaveVisualiserComponent> _waveVisualiser;
    juce::AudioBuffer<float> env_shapes;
    std::unique_ptr<EnvelopesInterpolator> envint;
    DoubleBuffer* envelopeDB;

	bool stateRestored = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CustomAudioProcessor)
};