#pragma once
#include "ApiRoutes.h"
#include "../Utils/StringUtils.h"
#include "../ThirdParty/json.hpp"

class ApiService
{
public:
    static juce::String makeGETRequest(ApiRoute route);
    static juce::String makePOSTRequest(ApiRoute route, const nlohmann::json& body);

private:
    template <typename RequestConfig>
    static juce::String makeHttpRequest(ApiRoute route, RequestConfig configureRequest);
    static juce::URL::InputStreamOptions buildOptions();
    static juce::String buildApiUrl(ApiRoute route);
    ApiService() {}
    ApiService& operator=(const ApiService&) = delete;
};