#pragma once

#include <iostream>
#include <rtc/rtc.hpp>
#include "../Utils/VectorUtils.h"
#include <juce_core/juce_core.h>

#include "../Common/EventListener.h"
#include "../Api/SocketEvents.h"
#include "../Api/SocketRoutes.h"
#include "../Common/ReconnectTimer.h"
#include "../Rtc/WebRTCConnexionState.h"


class WebRTCSenderConnexionHandler: public WebRTCConnexionState {
public:
    explicit WebRTCSenderConnexionHandler(WsRoute wsRoute);
    void setupConnection() override;
protected:
    std::shared_ptr<rtc::Track> audioTrack;
private:
    void setOffer();
    void handleAnswer(const std::string& sdp);
    void startAnswerReceivedCheckTimer();
    void onWsMessageReceived(const MessageWsReceivedEvent &event) override;

    // Answer monitoring
    bool answerReceived = false;
    const int maxResendAttempts = 100;
    int resendAttempts = 0;
    int resendIntervalMs = 10000;
    std::optional<ReconnectTimer> answerTimer;
};
