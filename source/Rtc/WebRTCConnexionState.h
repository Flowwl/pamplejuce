#pragma once

#include <iostream>
#include <rtc/rtc.hpp>
#include <juce_core/juce_core.h>

#include "../Api/WebSocketService.h"
#include "../Common/EventListener.h"
#include "../Models/Session.h"
#include "../Api/SocketRoutes.h"
#include "../Common/ReconnectTimer.h"

class WebRTCConnexionState: public virtual EventListener {
public:
    explicit WebRTCConnexionState(WsRoute wsRoute);
    ~WebRTCConnexionState() override;

    virtual void setupConnection() = 0;
    virtual void disconnect() const;
    virtual void resetConnection();

    [[nodiscard]] bool isConnected() const;
    [[nodiscard]] bool isConnecting() const;

    [[nodiscard]] juce::String getSignalingStateLabel() const;
    [[nodiscard]] juce::String getIceCandidateStateLabel() const;

protected:
    std::shared_ptr<rtc::PeerConnection> peerConnection;
    void notifyRTCStateChanged() const;
    void onOngoingSessionChanged(const OngoingSessionChangedEvent& event) override;

    bool sendCandidateToRemote(const rtc::Candidate& candidate);
    bool sendOfferToRemote(const rtc::Description &sdp);
    bool sendAnswerToRemote(const rtc::Description &sdp);
    void attemptReconnect();

    // Ice Reconnection
    int reconnectAttempts = 0;
    const int maxReconnectAttempts = 5;
    const int reconnectDelayMs = 2000;
    ReconnectTimer reconnectTimer;
    std::vector<rtc::Candidate> pendingCandidates;

private:
    WebSocketService meloWebSocketService;
    std::optional<PopulatedSession> ongoingSession;
};


