#pragma once
#include <juce_audio_devices/juce_audio_devices.h>

class DebugAudioCallback : public juce::AudioIODeviceCallback
{
public:
    DebugAudioCallback() : currentSampleRate(0.0), currentAngle(0.0), angleDelta(0.0) {}

    void audioDeviceIOCallback(const float** inputChannelData, int numInputChannels,
                               float** outputChannelData, const int numOutputChannels,
                               const int numSamples)
    {
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            auto* channelData = outputChannelData[channel];
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const auto currentSample = static_cast<float>(std::sin(currentAngle));
                currentAngle += angleDelta;

                channelData[sample] = currentSample;
            }
        }
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        currentSampleRate = device->getCurrentSampleRate();
        angleDelta = juce::MathConstants<double>::twoPi * frequency / currentSampleRate;
    }

    void audioDeviceStopped() override
    {
        currentSampleRate = 0.0;
    }

private:
    double currentSampleRate;
    double currentAngle;
    double angleDelta;
    const double frequency = 440.0; // Fréquence en Hz (La 440)
};
