#pragma once

#include <string>
#include "../Utils/StringUtils.h"
#include "../ThirdParty/json.hpp"

class SocketEvent {
public:
    virtual ~SocketEvent() = default;

    std::string type;
    virtual std::string createMessage() = 0;
};

class SellerConnectedEvent final : SocketEvent {
public:
    SellerConnectedEvent(const std::string &sellerUserId, const std::string &sessionId, const std::string &artistUserId) : sellerUserId(sellerUserId), sessionId(sessionId), artistUserId(artistUserId) {}
    std::string type = "seller-connected";
    std::string sellerUserId;
    std::string sessionId;
    std::string artistUserId;
    std::string createMessage() override {
        return StringUtils::createWsMessage(type, {
            {"sellerUserId", sellerUserId},
            {"sessionId", sessionId},
            {"artistUserId", artistUserId}
        });
    }
};

class RTCOfferSentEvent final : SocketEvent {
public:
    RTCOfferSentEvent(const rtc::Description &sdp, const PopulatedSession &ongoingSession) : sdp(sdp), ongoingSession(ongoingSession) {}
    std::string type = "offer";
    rtc::Description sdp;
    PopulatedSession ongoingSession;
    std::string createMessage() override {
        return StringUtils::createWsMessage(type, {
        {"sdp", sdp.generateSdp()},
        {"sessionId", ongoingSession._id},
        {"sellerUserId", ongoingSession.seller.user._id},
        {"artistUserId", ongoingSession.reservedByArtist.user._id}
        });
    }
};
class RTCAnswerSentEvent final : SocketEvent {
public:
    std::string type = "answer";
    std::string sdp;
    PopulatedSession ongoingSession;
    explicit RTCAnswerSentEvent(const std::string &sdp, const PopulatedSession &ongoingSession) : sdp(sdp), ongoingSession(ongoingSession) {}
    std::string createMessage() override {
        return StringUtils::createWsMessage(type, {
            {"sdp", sdp },
            {"sessionId", ongoingSession._id},
            {"sellerUserId", ongoingSession.seller.user._id},
            {"artistUserId", ongoingSession.reservedByArtist.user._id}
        });
    }
};

class RTCAnswerReceivedEvent final : SocketEvent {
public:
    std::string type = "answer";
    std::string sdp;
    PopulatedSession ongoingSession;
    explicit RTCAnswerReceivedEvent(const std::string &sdp, const PopulatedSession &ongoingSession) : sdp(sdp), ongoingSession(ongoingSession) {}
    std::string createMessage() override {
        return StringUtils::createWsMessage(type, {
            {"sdp", sdp },
            {"sessionId", ongoingSession._id},
            {"sellerUserId", ongoingSession.seller.user._id},
            {"artistUserId", ongoingSession.reservedByArtist.user._id}
        });
    }
};

class RTCIceCandidateSentEvent final : SocketEvent {
public:
    std::string type = "ice-candidate";
    rtc::Candidate candidate;
    PopulatedSession ongoingSession;
    explicit RTCIceCandidateSentEvent(const rtc::Candidate &candidate, const PopulatedSession &ongoingSession) : candidate(candidate), ongoingSession(ongoingSession) {}
    std::string createMessage() override {
        return StringUtils::createWsMessage(type, {
            {"meId", ongoingSession.seller.user._id },
            {"candidate", candidate.candidate()},
            {"sdpMid", candidate.mid()},
            {"sdpMLineIndex", candidate.priority()},
            {"sessionId", ongoingSession._id},
            {"sellerUserId", ongoingSession.seller.user._id},
            {"artistUserId", ongoingSession.reservedByArtist.user._id}
        });
    }
};

class RTCIceCandidateReceivedEvent final : SocketEvent {
public:
    std::string type = "ice-candidate";
    std::string candidate;
    PopulatedSession ongoingSession;
    explicit RTCIceCandidateReceivedEvent(const std::string &candidate, const PopulatedSession &ongoingSession) : candidate(candidate), ongoingSession(ongoingSession) {}
    std::string createMessage() override {
        return StringUtils::createWsMessage(type, {
            {"meId", ongoingSession.seller.user._id },
            {"candidate", candidate},
            {"sessionId", ongoingSession._id},
            {"sellerUserId", ongoingSession.seller.user._id},
            {"artistUserId", ongoingSession.reservedByArtist.user._id}
        });
    }
};