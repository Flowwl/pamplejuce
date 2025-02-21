#pragma once

#include <optional>
#include "Artist.h"
#include "User.h"
#include "Seller.h"
#include "../Utils/StringUtils.h"

class UserContext
{
public:
    std::optional<Artist> artist;
    std::optional<Seller> seller;
    User user;

    static UserContext fromJsonString(const juce::String& jsonString) {
        const auto keyPair = juce::JSON::parse(jsonString);

        UserContext userContext;
        userContext.user = User::fromJsonString(juce::JSON::toString(keyPair.getProperty("user", "{}")));
        userContext.seller = Seller::fromJsonString(juce::JSON::toString(keyPair.getProperty("seller", "{}")));
        userContext.artist = Artist::fromJsonString(juce::JSON::toString(keyPair.getProperty("artist", "{}")));
        return userContext;
    }
};