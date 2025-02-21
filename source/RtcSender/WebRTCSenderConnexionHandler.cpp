#include "WebRTCSenderConnexionHandler.h"
#include "../Common/EventManager.h"
#include "../ThirdParty/json.hpp"
#include "../AudioSettings.h"
#include "../Api/SocketRoutes.h"

WebRTCSenderConnexionHandler::WebRTCSenderConnexionHandler(const WsRoute wsRoute): WebRTCConnexionState(wsRoute){
}

void WebRTCSenderConnexionHandler::setupConnection() {
    rtc::InitLogger(rtc::LogLevel::Info);
    rtc::Configuration config;
    config.iceServers.emplace_back("stun:stun.l.google.com:19302");

    peerConnection = std::make_shared<rtc::PeerConnection>(config);

    peerConnection->onLocalDescription([this](const rtc::Description &sdp) {
        juce::Logger::outputDebugString("Local description set");
        if (!peerConnection->remoteDescription()) {
            juce::Logger::outputDebugString("No remote description. Waiting for answer.");
            if (sendOfferToRemote(sdp)) {
                startAnswerReceivedCheckTimer();
            }
        }
    });

    peerConnection->onLocalCandidate([this](const rtc::Candidate &candidate) {
        if (peerConnection->signalingState() == rtc::PeerConnection::SignalingState::Stable) {
            return;
        }

        if (peerConnection->remoteDescription()) {
            sendCandidateToRemote(candidate);
        } else {
            pendingCandidates.push_back(candidate);
            juce::Logger::outputDebugString("Candidate stored temporarily. Waiting for remote description.");
        }
    });

    peerConnection->onStateChange([this](rtc::PeerConnection::State state) {
        notifyRTCStateChanged();
    });

    peerConnection->onSignalingStateChange([this](rtc::PeerConnection::SignalingState state) {
        notifyRTCStateChanged();
    });

    peerConnection->onTrack([this](const std::shared_ptr<rtc::Track> &track) {
        juce::Logger::outputDebugString("Track received");
        audioTrack = track;
    });

    peerConnection->onIceStateChange([this](const rtc::PeerConnection::IceState state) {
        notifyRTCStateChanged();
        switch (state) {
            case rtc::PeerConnection::IceState::New:
                juce::Logger::outputDebugString("ICE state changed to: New");
                break;
            case rtc::PeerConnection::IceState::Closed:
            case rtc::PeerConnection::IceState::Disconnected: {
                break;
            }
            case rtc::PeerConnection::IceState::Failed: {
                juce::Logger::outputDebugString("ICE state changed to: Disconnected / Failed / Closed");
                ReconnectTimer::callAfterDelay(reconnectDelayMs, [this]() { attemptReconnect(); });
                break;
            }
            case rtc::PeerConnection::IceState::Checking:
                break;
            case rtc::PeerConnection::IceState::Completed:
            case rtc::PeerConnection::IceState::Connected: {
                if (peerConnection->state() == rtc::PeerConnection::State::Connected) {
                    juce::Logger::outputDebugString("Already connected, skipping reconnection.");
                    return;
                }
                reconnectAttempts = 0;
                juce::Logger::outputDebugString("ICE state changed to: Connected");
                break;
            }
            default:
                break;
        }
    });

    rtc::Description::Audio newAudioTrack{};
    newAudioTrack.addOpusCodec(111, "minptime=10;useinbandfec=0");
    newAudioTrack.setBitrate(AudioSettings::getInstance().getOpusBitRate()); // Débit binaire en bits par seconde
    newAudioTrack.setDirection(rtc::Description::Direction::SendOnly);
    newAudioTrack.addSSRC(12345, "CNAME");;
    audioTrack = peerConnection->addTrack(static_cast<rtc::Description::Media>(newAudioTrack));
    setOffer();
}


void WebRTCSenderConnexionHandler::onWsMessageReceived(const MessageWsReceivedEvent &event) {
    if (!peerConnection) {
        return;
    }
    if (peerConnection->signalingState() != rtc::PeerConnection::SignalingState::HaveLocalOffer) {
        return;
    }

    if (event.type == "answer" && event.data.contains("sdp")) {
        handleAnswer(event.data["sdp"]);
    } else if (event.type == "ice-candidate") {
        if (peerConnection->state() == rtc::PeerConnection::State::Connected) {
            return;
        }
        juce::Logger::outputDebugString("Received ICE candidate ->" + event.data["candidate"]);
        const std::string candidate = event.data["candidate"];
        const std::string sdpMid = event.data["sdpMid"];

        const auto iceCandidate(rtc::Candidate(candidate, sdpMid));
        peerConnection->addRemoteCandidate(iceCandidate);
    }
}

void WebRTCSenderConnexionHandler::setOffer() {
    juce::Logger::outputDebugString("Setting offer");
    if (peerConnection->signalingState() != rtc::PeerConnection::SignalingState::Stable && peerConnection->state() ==
        rtc::PeerConnection::State::Connected) {
        return;
    }
    if (peerConnection->localDescription().has_value()) {
        juce::Logger::outputDebugString("Offer already set, sending it to remote");
        sendOfferToRemote(peerConnection->localDescription().value());
    } else {
        peerConnection->localDescription().reset();
        peerConnection->setLocalDescription(rtc::Description::Type::Offer);
    }
}

void WebRTCSenderConnexionHandler::handleAnswer(const std::string &sdp) {
    if (peerConnection->signalingState() != rtc::PeerConnection::SignalingState::HaveLocalOffer) {
        return;
    }
    if (peerConnection->state() == rtc::PeerConnection::State::Connected) {
        return;
    }

    if (peerConnection->remoteDescription()) {
        return;
    }

    juce::Logger::outputDebugString("Received answer");
    peerConnection->setRemoteDescription(rtc::Description(sdp, "answer"));
    answerReceived = true;
    for (const auto &candidate: pendingCandidates) {
        sendCandidateToRemote(candidate);
    }
    pendingCandidates.clear();
}

void WebRTCSenderConnexionHandler::startAnswerReceivedCheckTimer() {
    if (!answerTimer.has_value()) {
        answerTimer.emplace(ReconnectTimer([this]() {
            if (answerReceived) {
                juce::Logger::outputDebugString("Answer received. Stopping the timer.");
                answerTimer->stopTimer();
                return;
            }

            if (resendAttempts >= maxResendAttempts) {
                juce::Logger::outputDebugString("Max resend attempts reached. Stopping resends.");
                answerTimer->stopTimer(); // Arrêter le timer après trop de tentatives
                return;
            }

            juce::Logger::outputDebugString("Resending offer...");
            resetConnection();
            resendAttempts++;
        }));
    }
    answerReceived = false;
    resendAttempts = 0;

    answerTimer->startTimer(resendIntervalMs);
}