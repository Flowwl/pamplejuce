#pragma once
#include "Events.h"

class EventListener
{
public:
    virtual ~EventListener() = default;
    virtual void onAudioBlockProcessedEvent(const AudioBlockProcessedEvent& event) {}
    virtual void onAudioBlockSent(const AudioBlockSentEvent& event) {}
    virtual void onAudioBlockReceived(const AudioBlockReceivedEvent& event) {}
    virtual void onAudioBlockReceivedDecoded(const AudioBlockReceivedDecodedEvent& event) {}
    virtual void onLoginEvent(const LoginEvent& event) {}
    virtual void onLogoutEvent(const LogoutEvent& event) {}
    virtual void onOngoingSessionChanged(const OngoingSessionChangedEvent& event) {}
    virtual void onWsMessageReceived(const MessageWsReceivedEvent& event) {}
    virtual void onRTCStateChanged(const RTCStateChangeEvent& event) {}
};
