#pragma once
#include <exception>
#include "../Api/ApiService.h"
#include "../Api/AuthService.h"
#include <juce_core/juce_core.h>

class CrashHandler {
public:
    static void reportCrash(const std::string& message) {
        nlohmann::json jsonBody = {
            {"message", message},
            {"platform", "juce_send"},
            {"stackTrace", message},
            {"screenName", "MainApp"},
        };
        if (AuthService::getInstance().getUserContext().has_value() ) {
            jsonBody["userId"] = AuthService::getInstance().getUserContext().value().user._id;
        }
        auto res = ApiService::makePOSTRequest(ApiRoute::CreateCrashReport, jsonBody);
    }

    static void customTerminateHandler() {
        try {
            std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            reportCrash(e.what());
        } catch (...) {
            reportCrash("Unknown exception occurred");
        }
        juce::Logger::outputDebugString("Crash report sent");

        // Appeler le gestionnaire par défaut après votre rapport
        std::abort();
    }
};