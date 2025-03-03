#pragma once
#include <iostream>
#include <juce_core/juce_core.h>
#include "../Config.h"
#include "../Common/ResamplerWrapper.h"
#include "../Common/OpusEncoderWrapper.h"
#include "../Common/RTPWrapper.h"
#include <rtc/rtc.hpp>

class WebRTCAudioSenderSendingHandler {
public:
    WebRTCAudioSenderSendingHandler(std::shared_ptr<rtc::Track> audioTrack);
    void sendAudioData (const std::vector<float>& accumulationBuffer);
    void refreshFrameSamples();
    int getDawFrameSamplesPerChannel() const { return dawFrameSamplesPerChannel; }
    int getDawFrameSamplesWithAllChannels() const { return dawFrameSamplesWithAllChannels; }
private:
    void sendOpusPacket(const std::vector<unsigned char>& opusPacket);
    std::shared_ptr<rtc::Track> audioTrack;
    OpusEncoderWrapper opusEncoder;
    ResamplerWrapper resampler;
    uint16_t seqNum = 1;
    uint32_t timestamp = 0;
    juce::int64 lastSentToRTCTime = juce::Time::currentTimeMillis();

    int dawFrameSamplesPerChannel = 0;
    int dawFrameSamplesWithAllChannels = 0;

};