#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <fstream>

#include "Common/CircularBuffer.h"
#include "Common/EventListener.h"
#include <deque>

// struct AudioPacket {
//     uint64_t timestamp;
//     std::vector<float> data;
// };
//==============================================================================
class MainAudioProcessor final : public juce::AudioProcessor, EventListener
{
public:
    //==============================================================================
    MainAudioProcessor();
    ~MainAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainAudioProcessor);

#ifdef IN_RECEIVING_MODE
    void onAudioBlockReceivedDecoded(const AudioBlockReceivedDecodedEvent &event) override;
    std::deque<std::vector<float>> audioQueue;
    juce::CriticalSection lock;
    std::vector<float> currentBlock;
    size_t currentSampleIndex = 0;
    double currentSampleRate = 0;
#endif

};
