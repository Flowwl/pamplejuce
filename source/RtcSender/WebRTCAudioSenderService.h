#pragma once

#include <iostream>
#include <juce_core/juce_core.h>

#include "../Common/OpusEncoderWrapper.h"
#include "../Api/WebSocketService.h"
#include "../Common/EventListener.h"

#include "../Common/ResamplerWrapper.h"
#include "WebRTCSenderConnexionHandler.h"
#include "../Common/CircularBuffer.h"

struct CompareAudioEvent {
    bool operator()(const std::shared_ptr<AudioBlockProcessedEvent> &a,
                    const std::shared_ptr<AudioBlockProcessedEvent> &b) const {
        // Plus ancien en haut de la file
        return a->timestamp > b->timestamp;
    }
};

class WebRTCAudioSenderService final : public WebRTCSenderConnexionHandler {
public:
    WebRTCAudioSenderService();

    ~WebRTCAudioSenderService() override;

private:
    void stopAudioThread();

    void startAudioThread();

    void onRTCStateChanged(const RTCStateChangeEvent &event) override;

    void onAudioBlockProcessedEvent(const AudioBlockProcessedEvent &event) override;

    void sendOpusPacket(const std::vector<unsigned char>& opusPacket,uint32_t packetTimestamp,int frameSamples);

    void processingThreadFunction();

    OpusEncoderWrapper opusEncoder;
    ResamplerWrapper resampler;
    uint16_t seqNum = 1;
    uint32_t timestamp = 0;
    uint32_t ssrc = 12345;

    std::deque<AudioBlockProcessedEvent> audioQueue;

    std::atomic<bool> threadRunning{true};
    std::thread encodingThread;

    std::mutex audioQueueMutex;
    std::priority_queue<
        std::shared_ptr<AudioBlockProcessedEvent>,
        std::vector<std::shared_ptr<AudioBlockProcessedEvent> >,
        CompareAudioEvent
    > audioEventQueue;
    juce::CriticalSection dequeLock;
};
