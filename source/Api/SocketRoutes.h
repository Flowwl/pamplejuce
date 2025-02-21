#pragma once
#include <juce_core/juce_core.h>

enum class WsRoute
{
    GetOngoingSession,
    GetOngoingSessionRTCInstru,
    GetOngoingSessionRTCVoice,
};


inline juce::String getWsRouteString(const WsRoute route)
{
    switch (route)
    {
        case WsRoute::GetOngoingSession:            return "/ongoing-session";
        case WsRoute::GetOngoingSessionRTCInstru:            return "/ongoing-session-rtc-instru";
        case WsRoute::GetOngoingSessionRTCVoice:            return "/ongoing-session-rtc-voice";
        default:                                    return "";
    }
}