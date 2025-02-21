#pragma once
#include <rtc/rtc.hpp>

#include "../Common/EventListener.h"
#include "../Api/SocketRoutes.h"
#include "../Rtc/WebRTCConnexionState.h"

class WebRTCReceiverConnexionHandler: public WebRTCConnexionState, public virtual EventListener {
public:
    explicit WebRTCReceiverConnexionHandler(WsRoute wsRoute);
    ~WebRTCReceiverConnexionHandler() override;
    void setupConnection() override;
protected:
    std::shared_ptr<rtc::Track> audioTrack;
private:
    void handleOffer(const std::string& sdp);
    void onWsMessageReceived(const MessageWsReceivedEvent &event) override;
};
