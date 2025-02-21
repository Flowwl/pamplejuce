#include "AuthService.h"
#include "ApiService.h"
#include "../Utils/StringUtils.h"
#include "../Common//JuceLocalStorage.h"
#include "../Common/EventManager.h"

AuthService &AuthService::getInstance() {
    static AuthService instance;
    return instance;
}

juce::String AuthService::login(const juce::String &email, const juce::String &password) {
    const nlohmann::json jsonBody = {
        {"email",    email.toStdString()},
        {"password", password.toStdString()}
    };

    try {
        auto res = ApiService::makePOSTRequest(ApiRoute::PostLogin, jsonBody);
        if (res.isEmpty()) {
            return res;
        }
        auto accessToken = StringUtils::parseJsonStringToKeyPair(res).getValue("access_token", "");
        JuceLocalStorage::getInstance().saveValue("access_token", accessToken);
        fetchUserContext();
        EventManager::getInstance().notifyLogin(accessToken);
        return accessToken;
    } catch (std::exception &e) {
        juce::Logger::outputDebugString("Login failed : " + juce::String(e.what()));
        logout();
        return "";
    }
}
std::optional<UserContext> AuthService::fetchUserContext() {
    const auto myUserContext = ApiService::makeGETRequest(ApiRoute::GetMyUserContext);
    if (myUserContext.isEmpty()) {
        juce::Logger::outputDebugString("Aucun UserContext trouvé");
        logout();
        return std::nullopt;
    }
    userContext = UserContext().fromJsonString(myUserContext);
    return userContext;
}

void AuthService::logout() {
    JuceLocalStorage::getInstance().removeValue("access_token");
    EventManager::getInstance().notifyLogout();
}

std::optional<UserContext> AuthService::getUserContext() const {
    return userContext;
}
