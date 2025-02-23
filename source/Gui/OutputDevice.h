#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Common/EventListener.h"
#include "../Common/EventManager.h"

class OutputDevice: public juce::AudioAppComponent, public EventListener {
public:
    OutputDevice(): juce::AudioAppComponent(audioDeviceManager) {
        audioDeviceManager.initialise (0, 2, nullptr, true, "MeloVST Virtual Output");
        audioSettings = std::make_unique<juce::AudioDeviceSelectorComponent>(audioDeviceManager, 0, 2, 0, 2, true, true, true, false);
        setAudioChannels(0, 2); // 0 entrées, 2 sorties
        EventManager::getInstance().addListener(this);
    }
    ~OutputDevice() override
    {
        shutdownAudio();
        EventManager::getInstance().removeListener (this);
    }
    juce::AudioDeviceSelectorComponent* getAudioSettings() const
    {
        return audioSettings.get();
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {}
    void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override {}
    void releaseResources() override {}
private:
    juce::AudioDeviceManager audioDeviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSettings;

};