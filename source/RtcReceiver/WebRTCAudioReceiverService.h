#pragma once

#include <iostream>
#include <juce_core/juce_core.h>

#include "../Common/OpusEncoderWrapper.h"
#include "../Api/WebSocketService.h"
#include "../Common/EventListener.h"

#include "../Common/ResamplerWrapper.h"
#include "WebRTCReceiverConnexionHandler.h"

class WebRTCAudioReceiverService final : public WebRTCReceiverConnexionHandler {
public:
    WebRTCAudioReceiverService();
    ~WebRTCAudioReceiverService() override;

private:
    void onAudioBlockReceived(const AudioBlockReceivedEvent &event) override;
    std::vector<float> decodeNextAudioPacket();
    OpusEncoderWrapper opusCodec;
    OpusDecoder* decoder;
    ResamplerWrapper resampler;
    // Buffer pour stocker les paquets audio triés par timestamp
    std::map<uint64_t, rtc::binary> audioBuffer;
    std::mutex bufferMutex;
};
