#include "WebRTCConnexionState.h"
#include "../Common/EventManager.h"
#include "../ThirdParty/json.hpp"
#include "../AudioSettings.h"
#include "../Api/SocketRoutes.h"
#include "../Api/SocketEvents.h"

WebRTCConnexionState::WebRTCConnexionState(const WsRoute wsRoute): reconnectTimer([this]() { attemptReconnect(); }),
    meloWebSocketService(
        WebSocketService(getWsRouteString(wsRoute))) {
    EventManager::getInstance().addListener(this);
}

WebRTCConnexionState::~WebRTCConnexionState() {
    EventManager::getInstance().removeListener(this);
    if (peerConnection) {
        peerConnection->close();
    }
}

void WebRTCConnexionState::disconnect() const {
    if (peerConnection) {
        peerConnection->close();
    }
}

void WebRTCConnexionState::resetConnection() {
    disconnect();
    setupConnection();
}

void WebRTCConnexionState::notifyRTCStateChanged() const {
    juce::Logger::outputDebugString("RTC state changed, notifying listeners");
    EventManager::getInstance().notifyOnRTCStateChanged({
        peerConnection->state(), peerConnection->iceState(), peerConnection->signalingState()
    });
}

void WebRTCConnexionState::onOngoingSessionChanged(const OngoingSessionChangedEvent &event) {
    ongoingSession = event.ongoingSession;
    if (meloWebSocketService.isConnected()) {
        return;
    }
    meloWebSocketService.connectToServer();
}

void WebRTCConnexionState::attemptReconnect() {
    if (!ongoingSession.has_value()) {
        juce::Logger::outputDebugString("No ongoing session. Cannot reconnect.");
        return;
    }
    if (peerConnection && peerConnection->state() == rtc::PeerConnection::State::Connected) {
        juce::Logger::outputDebugString("Already connected. Skipping reconnection.");
        return;
    }
    if (reconnectAttempts >= maxReconnectAttempts) {
        juce::Logger::outputDebugString("Max reconnect attempts reached. Giving up.");
        return;
    }

    juce::Logger::outputDebugString("Attempting to reconnect...");
    reconnectAttempts++;

    resetConnection();

    reconnectTimer.callAfterDelay(reconnectDelayMs, [this]() {
        if (!peerConnection || peerConnection->state() == rtc::PeerConnection::State::Closed) {
            attemptReconnect();
        } else {
            juce::Logger::outputDebugString("Reconnection successful.");
            reconnectAttempts = 0;
        }
    });
}

bool WebRTCConnexionState::isConnected() const {
    if (!peerConnection) {
        return false;
    }
    return peerConnection->state() == rtc::PeerConnection::State::Connected;
}

bool WebRTCConnexionState::isConnecting() const {
    if (!peerConnection) {
        return false;
    }
    return peerConnection->state() == rtc::PeerConnection::State::Connecting;
}

bool WebRTCConnexionState::sendAnswerToRemote(const rtc::Description &sdp) {
    if (ongoingSession.has_value()) {
        const auto offerEvent = new RTCAnswerSentEvent(sdp, ongoingSession.value());
        meloWebSocketService.sendMessage(offerEvent->createMessage());
        return true;
    }
    return false;
}

bool WebRTCConnexionState::sendOfferToRemote(const rtc::Description &sdp) {
    if (peerConnection->signalingState() != rtc::PeerConnection::SignalingState::HaveLocalOffer
        || peerConnection->state() == rtc::PeerConnection::State::Connected) {
        return false;
    }
    if (ongoingSession.has_value()) {
        const auto offerEvent = new RTCOfferSentEvent(sdp, ongoingSession.value());
        meloWebSocketService.sendMessage(offerEvent->createMessage());
        return true;
    }
    return false;
}

bool WebRTCConnexionState::sendCandidateToRemote(const rtc::Candidate &candidate) {
    if (!ongoingSession.has_value()) {
        return false;
    }
    juce::Logger::outputDebugString("Sending candidate to remote");
    const auto candidateEvent = new RTCIceCandidateSentEvent(candidate, ongoingSession.value());
    meloWebSocketService.sendMessage(candidateEvent->createMessage());
    return true;
}

juce::String WebRTCConnexionState::getSignalingStateLabel() const {
    if (!peerConnection) {
        return juce::String::fromUTF8("Inconnu");
    }
    switch (peerConnection->signalingState()) {
        case rtc::PeerConnection::SignalingState::Stable:
            return juce::String::fromUTF8("Stable");
        case rtc::PeerConnection::SignalingState::HaveLocalOffer:
            return juce::String::fromUTF8("Offre locale");
        case rtc::PeerConnection::SignalingState::HaveRemoteOffer:
            return juce::String::fromUTF8("Offre distante");
        case rtc::PeerConnection::SignalingState::HaveLocalPranswer:
            return juce::String::fromUTF8("Pré-réponse locale");
        case rtc::PeerConnection::SignalingState::HaveRemotePranswer:
            return juce::String::fromUTF8("Pré-réponse distante");
        default:
            return juce::String::fromUTF8("Inconnu");
    }
}

juce::String WebRTCConnexionState::getIceCandidateStateLabel() const {
    if (!peerConnection) {
        return juce::String::fromUTF8("Inconnu");
    }
    switch (peerConnection->iceState()) {
        case rtc::PeerConnection::IceState::New:
            return juce::String::fromUTF8("Nouveau");
        case rtc::PeerConnection::IceState::Checking:
            return juce::String::fromUTF8("Vérification");
        case rtc::PeerConnection::IceState::Connected:
            return juce::String::fromUTF8("Connecté");
        case rtc::PeerConnection::IceState::Completed:
            return juce::String::fromUTF8("Complété");
        case rtc::PeerConnection::IceState::Failed:
            return juce::String::fromUTF8("Échoué");
        case rtc::PeerConnection::IceState::Disconnected:
            return juce::String::fromUTF8("Déconnecté");
        case rtc::PeerConnection::IceState::Closed:
            return juce::String::fromUTF8("Fermé");
        default:
            return juce::String::fromUTF8("Inconnu");
    }
}
