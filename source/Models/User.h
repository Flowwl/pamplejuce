#pragma once
#include <string>
#include <vector>


class User
{
public:
    std::string _id;
    std::string email;
    std::string firstname;
    std::string lastname;
    std::string nickname;
    std::string password;
    std::string userAlias;
    std::string phoneNumber;
    std::string cover;
    std::string birthdate;
    std::string ambassadorAt;
    bool isConfirmed;
    std::string confirmationCode;
    std::vector<std::string> mainLanguages;
    std::vector<std::string> spokenLanguages;
    std::vector<std::string> musicStylePreferences;
    std::string country;
    std::string createdAt;
    std::string updatedAt;

    static User fromJsonString(const juce::String& jsonString) {
        const auto keyPair = StringUtils::parseJsonStringToKeyPair(jsonString);
        User user;
        user._id = keyPair.getValue("_id", "").toStdString();
        user.email = keyPair.getValue("email", "").toStdString();
        user.firstname = keyPair.getValue("firstname", "").toStdString();
        user.lastname = keyPair.getValue("lastname", "").toStdString();
        user.nickname = keyPair.getValue("nickname", "").toStdString();
        user.userAlias = keyPair.getValue("userAlias", "").toStdString();
        user.phoneNumber = keyPair.getValue("phoneNumber", "").toStdString();
        user.cover = keyPair.getValue("cover", "").toStdString();
        user.birthdate = keyPair.getValue("birthdate", "").toStdString();
        user.ambassadorAt = keyPair.getValue("ambassadorAt", "").toStdString();
        user.isConfirmed = keyPair.getValue("isConfirmed", "false").toStdString() == "true";
        user.confirmationCode = keyPair.getValue("confirmationCode", "").toStdString();
        // user.mainLanguages = StringUtils::parseJsonStringToArray(keyPair.getValue("mainLanguages", "")).toStdString();
        // user.spokenLanguages = StringUtils::parseJsonStringToArray(keyPair.getValue("spokenLanguages", "")).toStdString();
        // user.musicStylePreferences = StringUtils::parseJsonStringToArray(keyPair.getValue("musicStylePreferences", "")).toStdString();
        user.country = keyPair.getValue("country", "").toStdString();
        user.createdAt = keyPair.getValue("createdAt", "").toStdString();
        user.updatedAt = keyPair.getValue("updatedAt", "").toStdString();
        return user;
    }
};