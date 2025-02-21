#pragma once

#include "MainAudioProcessor.h"
#include "Gui/MainWindow.h"
#include "Common/CrashHandler.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "Debug/DebugAudioCallback.h"

//==============================================================================
class MainApplication final : public juce::AudioProcessorEditor
{
public:
    explicit MainApplication (MainAudioProcessor&);
    ~MainApplication() override;


    void paint (juce::Graphics&) override;
    void resized() override;

private:
    std::unique_ptr<Component> mainWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainApplication)
};
