#pragma once

#include "../MainAudioProcessor.h"

//==============================================================================
class DebugApplication final : public juce::AudioProcessorEditor
{
public:
    DebugApplication (MainAudioProcessor& p): AudioProcessorEditor(&p) {
        setSize(600, 400);
    }
    ~DebugApplication() = default;


    void paint (juce::Graphics&)
    {

    };
    void resized()
    {

    };

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DebugApplication)
};
