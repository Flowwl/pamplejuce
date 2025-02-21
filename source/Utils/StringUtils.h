#pragma once
#include <juce_core/juce_core.h>

#include "../ThirdParty/json.hpp"

namespace StringUtils {
    static juce::String convertJsonToPOSTData(const nlohmann::json& jsonObject)
    {
        juce::StringArray parameterStrings;

        // Parcourir les paires clé-valeur de l'objet JSON
        for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
        {
            // Assurez-vous que la valeur est une chaîne ou convertible en chaîne
            if (it->is_string() || it->is_number() || it->is_boolean())
            {
                const juce::String key = juce::URL::addEscapeChars(it.key(), true); // Échapper la clé
                juce::String value = it->get<std::string>(); // Échapper la valeur

                parameterStrings.add(key + "=" + value);
            }
            else
            {
                throw std::invalid_argument("JSON values must be strings, numbers, or booleans.");
            }
        }

        // Joindre les paramètres dans le format key1=value1&key2=value2
        return parameterStrings.joinIntoString("&");
    }


    static juce::StringPairArray parseJsonStringToKeyPair(const juce::String& jsonString)
    {
        juce::StringPairArray keyPairArray;
        const juce::var parsedJson = juce::JSON::parse(jsonString);

        if (auto* jsonObject = parsedJson.getDynamicObject())
        {
            for (const auto & it : jsonObject->getProperties())
            {
                if (it.value.isObject()) {
                    juce::String key = it.name.toString();
                    juce::var value = juce::JSON::toString(it.value);
                    keyPairArray.set(key, value);
                }
                else {
                    juce::String key = it.name.toString();
                    juce::var value = it.value;
                    keyPairArray.set(key, value);
                }
            }
        }
        else
        {
            juce::Logger::outputDebugString("Invalid JSON or not an object: " + jsonString);
        }

        return keyPairArray;
    }

    inline std::string createWsMessage(const std::string& type, nlohmann::json data)
    {
        const nlohmann::json jsonMessage = {
            {"event", type},
            {"data", data}
        };
        return jsonMessage.dump();
    }
}
