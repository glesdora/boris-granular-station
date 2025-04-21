#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "JuceHeader.h"
#include <json/json.hpp>

#include "RNBO.h"
#include "RNBO_Utils.h"
#include "CustomAudioProcessor.h"
#include "CustomAudioEditor.h"
#include "BorisPalette.h"

#include <array>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

#define WIDTH 600
#define HEIGHT 400
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class MainContentComponent   : public Component, public RNBO::PatcherChangedHandler, public AsyncUpdater
{
public:
	//==============================================================================
	MainContentComponent()
	: _deviceSelectorComponent(_deviceManager,
		0,     // minimum input channels
		0,   // maximum input channels
		0,     // minimum output channels
		2,   // maximum output channels
		false, // ability to select midi inputs
		false, // ability to select midi output device
		false, // treat channels as stereo pairs
		false) // hide advanced options
    {	
		//_CrtSetBreakAlloc(15532);
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		DBG("About to load RNBO Audio Processor");

		loadRNBOAudioProcessor();

		RNBO::CoreObject& rnboObject = _audioProcessor->getRnboObject();

		_deviceManager.initialiseWithDefaultDevices(rnboObject.getNumInputChannels(), rnboObject.getNumOutputChannels());

		// setup our buffer size
		AudioDeviceManager::AudioDeviceSetup setup;
		_deviceManager.getAudioDeviceSetup(setup);
		setup.bufferSize = 128;
		_deviceManager.setAudioDeviceSetup(setup, false);

		_deviceManager.addAudioCallback(&_audioProcessorPlayer);

		// Only add the device selector if we're running as a standalone application
		if (JUCEApplicationBase::isStandaloneApp()) {
			// addAndMakeVisible (_deviceSelectorComponent);
			_includesDeviceSelector = true;
		}

		setSize (WIDTH+10, HEIGHT);
    }

	void patcherChanged() override
	{
		// we can't unload in the middle of the notification
		triggerAsyncUpdate();
	}

	void handleAsyncUpdate() override
	{
		// reload the processor
		loadRNBOAudioProcessor();
	}

	void loadRNBOAudioProcessor()
	{
		unloadRNBOAudioProcessor();

		jassert(_audioProcessor.get() == nullptr);

		_audioProcessor = std::unique_ptr<CustomAudioProcessor>(CustomAudioProcessor::CreateDefault());
		RNBO::CoreObject& rnboObject = _audioProcessor->getRnboObject();
		rnboObject.setPatcherChangedHandler(this);

		_audioProcessorPlayer.setProcessor(_audioProcessor.get());

		olmp = _deviceManager.getOutputLevelGetter();

		_audioProcessorEditor.reset(_audioProcessor->createEditorIfNeeded());
		if (_audioProcessorEditor) {
			addAndMakeVisible(_audioProcessorEditor.get());
			resized();  // set up the sizes
		}
	}

	void unloadRNBOAudioProcessor()
	{
		if (_audioProcessor) {
			_audioProcessorPlayer.setProcessor(nullptr);
			if (_audioProcessorEditor) {
				_audioProcessor->editorBeingDeleted(_audioProcessorEditor.get());
			}
			_audioProcessorEditor.reset();
			_audioProcessor.reset();
		}
	}

	void shutdownAudio()
	{
		unloadRNBOAudioProcessor();
		_deviceManager.removeAudioCallback(&_audioProcessorPlayer);
		_deviceManager.closeAudioDevice();
	}

	~MainContentComponent()
    {
		shutdownAudio();
    }


    //=======================================================================
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        g.fillAll (borisPalette[ground]);

        // You can add your drawing code here!
		auto r = olmp.get()->getCurrentLevel() * (double)getHeight() * 10.;
		Rectangle<int> monitorbounds = getLocalBounds().removeFromLeft(10);
		g.setColour(Colours::black);
		g.fillRect(monitorbounds);
		Rectangle<int> monitorLevel = Rectangle<int>(monitorbounds.getBottomLeft(), Point<int>(monitorbounds.getRight(), monitorbounds.getBottom()-(int)r));
		g.setColour(Colours::yellowgreen);
		g.fillRect(monitorLevel);

		repaint(monitorbounds);
    }

    void resized() override
    {
		const int selectorWidth = 328;
		int usedSelectorWidth = 0;

		if (_includesDeviceSelector) {
			usedSelectorWidth = /* std::min(getWidth(), selectorWidth); */ 10;
			// _deviceSelectorComponent.setBounds(0, 0, usedSelectorWidth, getHeight());
		}

		if (_audioProcessorEditor) {
			_audioProcessorEditor->setBounds(usedSelectorWidth, 0, getWidth() - usedSelectorWidth, getHeight());
		}
    }

private:
    //==============================================================================
	AudioDeviceManager		_deviceManager;
	AudioProcessorPlayer	_audioProcessorPlayer;

	std::unique_ptr<CustomAudioProcessor>		_audioProcessor;
	std::unique_ptr<AudioProcessorEditor>		_audioProcessorEditor;

	//IDEA: you initialize it every time the processor actually changes (could mean: when sample rate and buffer size change)
	std::unique_ptr<juce::AudioSampleBuffer> _10s_mirror_buffer;

	// Audio device chooser
	AudioDeviceSelectorComponent _deviceSelectorComponent;
	bool _includesDeviceSelector = false;

	AudioDeviceManager::LevelMeter::Ptr olmp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


// (This function is called by the app startup code to create our main component)
Component* createMainContentComponent()     { return new MainContentComponent(); }

#endif  // MAINCOMPONENT_H_INCLUDED
