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
        std::exception_ptr exptr = std::current_exception();
        if (exptr) { // Vérifier si une exception est active
            try {
                std::rethrow_exception(exptr);
            } catch (const std::exception& e) {
                reportCrash(e.what());
            } catch (...) {
                reportCrash("Unknown exception occurred");
            }
        } else {
            reportCrash("Terminate called without an active exception");
        }

        juce::Logger::outputDebugString("Crash report sent");
        std::abort();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrashHandler)

};