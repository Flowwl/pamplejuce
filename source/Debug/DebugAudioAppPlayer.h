#pragma once
#include "DebugAudioAppPlayer.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <deque>
#include "../Common/EventListener.h"
#include "../Common/EventManager.h"

class DebugAudioAppPlayer final : public juce::AudioAppComponent, public EventListener
{
public:
    DebugAudioAppPlayer()
    {
        setAudioChannels(0, 2); // 0 entrées, 2 sorties
        EventManager::getInstance().addListener(this);
    }

    ~DebugAudioAppPlayer() override
    {
        shutdownAudio();
        EventManager::getInstance().removeListener(this);
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        currentSampleRate = sampleRate;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill) override
    {
        if (bufferToFill.buffer == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        auto *leftChannel  = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
        auto *rightChannel = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
        int numSamples = bufferToFill.numSamples;
        int samplesWritten = 0;

        while (samplesWritten < numSamples)
        {
            juce::ScopedLock sl(lock);

            if (currentBlock.empty())
            {
                if (audioQueue.empty())
                {
                    // Remplir avec du silence si aucun bloc disponible
                    std::fill(leftChannel + samplesWritten, leftChannel + numSamples, 0.0f);
                    std::fill(rightChannel + samplesWritten, rightChannel + numSamples, 0.0f);
                    return;
                }
                else
                {
                    // Charger un nouveau bloc audio
                    currentBlock = std::move(audioQueue.front());
                    audioQueue.pop_front();
                    currentSampleIndex = 0;
                }
            }

            // Nombre de samples restant dans le bloc actuel
            int samplesRemainingInBlock = static_cast<int>(currentBlock.size()) - currentSampleIndex;
            int samplesToCopy = std::min(numSamples - samplesWritten, samplesRemainingInBlock);

            // Copier dans le buffer de sortie
            for (int i = 0; i < samplesToCopy; ++i)
            {
                float sampleValue = currentBlock[currentSampleIndex + i];
                leftChannel[samplesWritten + i]  = sampleValue;
                rightChannel[samplesWritten + i] = sampleValue; // Stéréo identique
            }

            samplesWritten += samplesToCopy;
            currentSampleIndex += samplesToCopy;

            if (currentSampleIndex >= currentBlock.size())
            {
                currentBlock.clear();
                currentSampleIndex = 0;
            }
        }
    }

    void releaseResources() override
    {
        // Libérer les ressources audio
    }

    void onAudioBlockReceivedDecoded(const AudioBlockReceivedDecodedEvent &event) {
        // juce::ScopedLock sl(lock);
        // audioQueue.push_back(event.data);
    }

private:
    std::deque<std::vector<float>> audioQueue;
    juce::CriticalSection lock;
    std::vector<float> currentBlock;
    size_t currentSampleIndex = 0;
    double currentSampleRate = 0;
};
