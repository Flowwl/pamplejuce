#pragma once

#include <iostream>
#include <juce_core/juce_core.h>

#include "../Common/OpusEncoderWrapper.h"
#include "../Api/WebSocketService.h"
#include "../Common/EventListener.h"

#include "../Common/ResamplerWrapper.h"
#include "WebRTCSenderConnexionHandler.h"
#include "WebRTCAudioSenderSendingHandler.h"

struct CompareAudioEvent {
    bool operator()(const std::shared_ptr<AudioBlockProcessedEvent> &a,
                    const std::shared_ptr<AudioBlockProcessedEvent> &b) const {
        // check null pointers
        if (!a || !b) {
            return false;
        }
        return a->timestamp > b->timestamp;
    }
};

class WebRTCAudioSenderService final : public WebRTCSenderConnexionHandler {
public:
    WebRTCAudioSenderService();

    ~WebRTCAudioSenderService() override;
    void disconnect();

private:
    void stopAudioThread();
    void startAudioThread();
    void onRTCStateChanged(const RTCStateChangeEvent &event) override;
    void onAudioBlockProcessedEvent(const AudioBlockProcessedEvent &event) override;
    void processAudio(std::vector<float> &accumulationBuffer);
    void processingThreadFunction();

    WebRTCAudioSenderSendingHandler sendingHandler;

    std::priority_queue<
        std::shared_ptr<AudioBlockProcessedEvent>,
        std::vector<std::shared_ptr<AudioBlockProcessedEvent> >,
        CompareAudioEvent
    > audioEventQueue;
    std::mutex audioQueueMutex;
    std::deque<AudioBlockProcessedEvent> audioQueue;

    std::thread encodingThread;
    std::mutex threadMutex;
    std::atomic<bool> threadRunning{false};
    std::atomic<bool> threadShouldExit { false };
    std::condition_variable cv;
    std::mutex cvMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WebRTCAudioSenderService)

};
