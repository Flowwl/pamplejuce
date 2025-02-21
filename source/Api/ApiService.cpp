#include "ApiService.h"
#include "../Common/JuceLocalStorage.h"

// Méthode GET
juce::String ApiService::makeGETRequest(const ApiRoute route)
{
    return makeHttpRequest(route, [](juce::URL& url, const juce::URL::InputStreamOptions& options) {
        options.withHttpRequestCmd("GET");
    });
}

// Méthode POST
juce::String ApiService::makePOSTRequest(const ApiRoute route, const nlohmann::json& body)
{
    return makeHttpRequest(route, [&body](juce::URL& url, const juce::URL::InputStreamOptions& options) {
        options.withHttpRequestCmd("POST")
              .withExtraHeaders("Content-Type: application/json\r\n");
        url = url.withPOSTData(StringUtils::convertJsonToPOSTData(body));
    });
}


template <typename RequestConfig>
juce::String ApiService::makeHttpRequest(const ApiRoute route, RequestConfig configureRequest)
{
    const juce::String apiUrl = buildApiUrl(route);
    juce::Logger::outputDebugString("Making HTTP request to " + apiUrl);

    try
    {
        juce::URL url(apiUrl);
        juce::URL::InputStreamOptions options = buildOptions();
        configureRequest(url, options);

        const std::unique_ptr<juce::InputStream> stream(url.createInputStream(options));
        if (stream == nullptr)
        {
            throw std::runtime_error("Unable to create input stream");
        }

        auto res = stream->readEntireStreamAsString();
        const auto jsonResponse = juce::JSON::parse(res);
        if (!jsonResponse.isObject())
        {
            juce::Logger::outputDebugString("Réponse non valide : " + res);
            return "";
        }

        if (jsonResponse["error"].toString().isNotEmpty()) {
            const juce::String message = juce::JSON::parse(res)["error"].toString();
            juce::Logger::outputDebugString("Exception lors de la requête HTTP : " + juce::String(res));
            return "";
        }

        const juce::var& statusCodeVar = jsonResponse["statusCode"];
        switch (int statusCode = static_cast<int>(statusCodeVar))
        {
            case 500:
            case 400:
            case 401:
            case 402:
            case 403:
            case 404:
            case 405:
            {
                // juce::String message = jsonResponse["error"].toString();
                // juce::Logger::outputDebugString("Exception lors de la requête HTTP : " + res);
                return "";
            }
            default:
                return res;
        }
    }
    catch (const std::exception& e)
    {
        juce::Logger::outputDebugString(juce::String::fromUTF8("Exception lors de la requête HTTP : "));
        juce::Logger::outputDebugString(e.what());
        return "";
    }
}

juce::URL::InputStreamOptions ApiService::buildOptions()
{
    if (const auto accessToken = JuceLocalStorage::getInstance().loadValue("access_token"); !accessToken.isEmpty()) {
        constexpr juce::URL::ParameterHandling parameterHandling{};
        const juce::URL::InputStreamOptions options(parameterHandling);
        return options.withExtraHeaders("Authorization: Bearer " + accessToken);
        ;
    }

    constexpr juce::URL::ParameterHandling parameterHandling{};
    juce::URL::InputStreamOptions options(parameterHandling);
    return options;
}

// Méthode pour construire l'URL complète
juce::String ApiService::buildApiUrl(const ApiRoute route)
{
    return Config::apiUrl + getApiRouteString(route);
}
