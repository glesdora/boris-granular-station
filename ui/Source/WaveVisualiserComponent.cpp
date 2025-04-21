#include "WaveVisualiserComponent.h"

WaveVisualiserComponent::WaveVisualiserComponent(int num_channels) : juce::AudioVisualiserComponent(1) {
    //this->setRepaintRate(0);
}

void WaveVisualiserComponent::resized() {
    AudioVisualiserComponent::resized();
}

void WaveVisualiserComponent::clear() {
    juce::AudioVisualiserComponent::clear();
	mirrorbufToZero();
}

void WaveVisualiserComponent::pushInMirrorBuffer(const float* inputBuffer, int numSamples) {
    if (mirror_buffer == nullptr) return;

    auto writePointers = mirror_buffer->getArrayOfWritePointers();
    int mirrorbuffersize = mirror_buffer->getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        writePointers[0][write_pos] = inputBuffer[i];
        write_pos = (write_pos + 1) % mirrorbuffersize;
    }
}

//void WaveVisualiserComponent::reloadBuffer() {
//    if (mirror_buffer == nullptr) return;
//
//    auto mirrorbuffersize = mirror_buffer->getNumSamples();
//
//    /*for (int i = 0; i < mirrorbuffersize; ++i) {
//        float inputSample = mirror_buffer->getSample(0, (write_pos + i) % mirrorbuffersize);
//        this->pushSample(&inputSample, 1);
//    }*/
//
//	/* Trying to use pushBuffer(const float* const* channelData, int numChannels, int numSamples)
//	* 
//    */
//    auto readPointers = mirror_buffer->getArrayOfReadPointers()[0];
//
//}

void WaveVisualiserComponent::setPalette(Colour back, Colour scroll, Colour freeze) {
    background = back;
    wave_scrolling = scroll;
    wave_freezed = freeze;

    if (isFrozen.load()) {
        this->setColours(background, wave_freezed);
    } else {
        this->setColours(background, wave_scrolling);
    }
}

void WaveVisualiserComponent::scroll() {
    isFrozen.store(false);
    this->clear();
    this->setColours(background, wave_scrolling);
    this->setRepaintRate(framerate);
}

void WaveVisualiserComponent::freeze() {
    isFrozen.store(true);
    this->setRepaintRate(0);
    this->setColours(background, wave_freezed);
}

void WaveVisualiserComponent::prepareToDisplay(double samplerate, int vecsize) {
    if (samplerate <= 0) return;

    auto pxls = this->getWidth();
    if (!pxls) return;

    if (mirror_buffer == nullptr) {
        auto sampsxpxl = secs_displayed * samplerate / pxls;

        mirror_buffer.reset(new juce::AudioSampleBuffer(1, secs_displayed * samplerate));
		mirrorbufToZero();

        this->setRepaintRate(framerate);
        this->setSamplesPerBlock(sampsxpxl);
        this->setBufferSize(pxls);

		this->clear();
    }
}

void WaveVisualiserComponent::pushBlock(const float* input_buffer, int num_samples) {
    if (!isFrozen.load()) {
        this->pushInMirrorBuffer(input_buffer, num_samples);
        pushBuffer(&input_buffer, 1, num_samples);
    }
}

void WaveVisualiserComponent::mirrorbufToZero() {
	if (mirror_buffer == nullptr) return;

	auto writePointers = mirror_buffer->getArrayOfWritePointers();
	int mirrorbuffersize = mirror_buffer->getNumSamples();

	for (int i = 0; i < mirrorbuffersize; ++i) {
		writePointers[0][i] = 0.0f;
	}
}