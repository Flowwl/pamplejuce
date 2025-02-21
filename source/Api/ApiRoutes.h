#pragma once
#include <juce_core/juce_core.h>  // Pour manipuler les chaînes de caractères

enum class ApiRoute
{
    PostLogin,
    CreateCrashReport,
    GetMyUserContext,
    GetMyOngoingSessions,
    GetHealth
};

inline juce::String getApiRouteString(const ApiRoute route)
{
    switch (route)
    {
        case ApiRoute::PostLogin:             return "/auth/login";
        case ApiRoute::CreateCrashReport:             return "/crashes";
        case ApiRoute::GetMyUserContext:      return "/user-contexts";
        case ApiRoute::GetMyOngoingSessions:      return "/sessions/find-my-ongoing-sessions";
        case ApiRoute::GetHealth:             return "/health";
        default:                              return "";
    }
}