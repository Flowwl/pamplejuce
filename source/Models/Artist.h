#pragma once
#include <string>
#include "../Utils/StringUtils.h"
class  Artist
{
public:
    std::string _id;
    std::string userId;
    std::string createdAt;
    std::string updatedAt;

    static Artist fromJsonString(const juce::String& jsonString) {
        const auto keyPair = StringUtils::parseJsonStringToKeyPair(jsonString);
        Artist artist;

        artist._id = keyPair.getValue("_id", juce::String()).toStdString();
        artist.userId = keyPair.getValue("userId", juce::String()).toStdString();
        artist.createdAt = keyPair.getValue("createdAt", juce::String()).toStdString();
        artist.updatedAt = keyPair.getValue("updatedAt", juce::String()).toStdString();

        return artist;
    }
};