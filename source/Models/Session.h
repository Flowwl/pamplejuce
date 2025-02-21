#pragma once
#include <string>
#include <juce_core/juce_core.h>
#include "UserContext.h"

class Session
{
public:
  std::string _id;
  std::string sellerProductId;
  std::string reservedByArtistId;
  std::string bookingId;
  std::string sellerId;
  std::string startDate;
  std::string endDate;
  std::string stripePaymentId;
  std::string status;
  juce::Array<std::string> files;
  bool isTest;
};

class PopulatedSession : public Session
{
public:
  UserContext reservedByArtist;
  UserContext seller;

  static PopulatedSession fromJSON(const juce::var& json) {
    PopulatedSession session;
    session._id = json.getProperty("_id", "").toString().toStdString();
    session.sellerProductId = json.getProperty("sellerProductId", "").toString().toStdString();
    session.reservedByArtistId = json.getProperty("reservedByArtistId", "").toString().toStdString();
    session.bookingId = json.getProperty("bookingId", "").toString().toStdString();
    session.sellerId = json.getProperty("sellerId", "").toString().toStdString();
    session.startDate = json.getProperty("startDate", "").toString().toStdString();
    session.endDate = json.getProperty("endDate", "").toString().toStdString();
    session.stripePaymentId = json.getProperty("stripePaymentId", "").toString().toStdString();
    session.status = json.getProperty("status", "").toString().toStdString();
    session.isTest = json.getProperty("isTest", "false").toString().toStdString() == "true";
    session.reservedByArtist = UserContext::fromJsonString(juce::JSON::toString(json.getProperty("reservedByArtist", "{}")));
    session.seller = UserContext::fromJsonString(juce::JSON::toString(json.getProperty("seller", "{}")));
    return session;
  }

  static juce::Array<PopulatedSession> parseArrayFromJsonString(const juce::String& jsonString) {
    const auto json = juce::JSON::parse(jsonString);
    const auto array = json.getArray();
    juce::Array<PopulatedSession> sessions;
    for (int i = 0; i < array->size(); i++) {
      sessions.add(fromJSON(array->getReference(i)));
    }
    return sessions;
  }
};