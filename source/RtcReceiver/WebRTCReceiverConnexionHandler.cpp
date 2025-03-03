#include "WebRTCReceiverConnexionHandler.h"

#include "../Api/SocketRoutes.h"
#include "../Common/EventManager.h"
#include "../ThirdParty/json.hpp"
#include "../Utils/VectorUtils.h"
#include "../Config.h"
#include <opus.h>

WebRTCReceiverConnexionHandler::WebRTCReceiverConnexionHandler(const WsRoute wsRoute)
    : WebRTCConnexionState(wsRoute) {
    setupConnection();
}

WebRTCReceiverConnexionHandler::~WebRTCReceiverConnexionHandler() {
}

void WebRTCReceiverConnexionHandler::setupConnection() {
    rtc::Configuration config;
    config.iceServers.emplace_back(Config::getInstance().rtcStunServer);

    peerConnection = std::make_shared<rtc::PeerConnection>(config);

    peerConnection->onLocalDescription([this](const rtc::Description &sdp) {
        sendAnswerToRemote(sdp);
        juce::Logger::outputDebugString("Local description sent " + sdp.generateSdp());
    });

    peerConnection->onLocalCandidate([this](const rtc::Candidate &candidate) {
        juce::Logger::outputDebugString("Local candidate found");
        if (peerConnection->localDescription()) {
            sendCandidateToRemote(candidate);
            for (const auto &pendingCandidate: pendingCandidates) {
                sendCandidateToRemote(pendingCandidate);
            }
            pendingCandidates.clear();
        } else {
            pendingCandidates.push_back(candidate);
            juce::Logger::outputDebugString("Candidate stored temporarily. Waiting for remote description.");
        }
    });

    peerConnection->onStateChange([this](rtc::PeerConnection::State state) {
        notifyRTCStateChanged();
        // if (state == rtc::PeerConnection::State::Closed) {
        // peerConnection.reset();
        // }
    });

    peerConnection->onSignalingStateChange([this](rtc::PeerConnection::SignalingState state) {
        notifyRTCStateChanged();
    });

    peerConnection->onTrack([this](const std::shared_ptr<rtc::Track> &track) {
        juce::Logger::outputDebugString("Track received");
        audioTrack = track;
        track->onMessage([this](const rtc::message_variant &message) {
            auto chrono = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch());
            uint64_t timestamp = chrono.count();
            EventManager::getInstance().notifyOnAudioBlockReceived(AudioBlockReceivedEvent{message, timestamp});
        });
    });

    // Notification des changements d'état ICE
    peerConnection->onIceStateChange([this](const rtc::PeerConnection::IceState state) {
        notifyRTCStateChanged();
    });
}

void WebRTCReceiverConnexionHandler::onWsMessageReceived(const MessageWsReceivedEvent &event) {
    if (!peerConnection) {
        return;
    }
    if (event.type == "offer" && event.data.contains("sdp")) {
        handleOffer(event.data["sdp"]);
    } else if (event.type == "ice-candidate") {
        const std::string candidate = event.data["candidate"];
        const std::string sdpMid = event.data["sdpMid"];

        const auto iceCandidate = rtc::Candidate(candidate, sdpMid);
        peerConnection->addRemoteCandidate(iceCandidate);
        juce::Logger::outputDebugString("Remote candidate added");
    }
}

void WebRTCReceiverConnexionHandler::handleOffer(const std::string &sdp) {
    resetConnection();
    if (peerConnection->state() == rtc::PeerConnection::State::Connected) {
        return;
    }
    juce::Logger::outputDebugString("Offer received" + sdp);
    peerConnection->setRemoteDescription(rtc::Description(sdp, rtc::Description::Type::Offer));
}
