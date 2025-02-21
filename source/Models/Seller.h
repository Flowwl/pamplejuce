#pragma once
#include <string>

class Seller
{
public:
    std::string _id;
    std::string userId;
    std::string stripeAccountId;
    int yearsOfExperience;
    std::string profileDescription;
    int stars;
    int nbReviews;

    static Seller fromJsonString(const juce::String& jsonString) {
        const auto keyPair = StringUtils::parseJsonStringToKeyPair(jsonString);
        Seller seller;
        seller._id = keyPair.getValue("_id", "").toStdString();
        seller.userId = keyPair.getValue("userId", "").toStdString();
        seller.stripeAccountId = keyPair.getValue("stripeAccountId", "").toStdString();
        seller.yearsOfExperience = std::stoi(keyPair.getValue("yearsOfExperience", "0").toStdString());
        seller.profileDescription = keyPair.getValue("profileDescription", "").toStdString();
        seller.stars = std::stoi(keyPair.getValue("stars", "0").toStdString());
        seller.nbReviews = std::stoi(keyPair.getValue("nbReviews", "0").toStdString());
        return seller;
    }
};