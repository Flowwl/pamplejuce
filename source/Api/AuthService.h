#pragma once
#include <juce_core/juce_core.h>
#include "../Models/UserContext.h"

class AuthService
{
public:
    juce::String login(const juce::String& email, const juce::String& password);

    static void logout();
    static AuthService& getInstance();
    std::optional<UserContext> getUserContext() const;
    std::optional<UserContext> fetchUserContext();

private:
    std::optional<UserContext> userContext;
    AuthService() {}
    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;
};