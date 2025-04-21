#pragma once

#include <JuceHeader.h>

class WaveVisualiserComponent : public juce::AudioVisualiserComponent
{
public:
    WaveVisualiserComponent(int num_channels);

    void resized() override;
    void setPalette(Colour back, Colour scroll, Colour freeze);
    
    void clear();

    void prepareToDisplay(double samplerate, int vecsize);
    void pushBlock(const float* input_buffer, int num_samples);

    void scroll();
    void freeze();

private:
    const static int framerate = 30;
    const static int secs_displayed = 10;

    std::atomic<bool> isFrozen{ false };
    std::unique_ptr<juce::AudioSampleBuffer> mirror_buffer;
    int write_pos = 0;

    Colour background, wave_scrolling, wave_freezed;

    //void reloadBuffer();
    void pushInMirrorBuffer(const float* inputBuffer, int numSamples);
    void mirrorbufToZero();
};