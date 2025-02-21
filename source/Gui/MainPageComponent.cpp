//
// Created by Padoa on 08/01/2025.
//
#include "MainPageComponent.h"
#include "../Common/EventManager.h"
#include "../Api/ApiRoutes.h"
#include "../Api/SocketRoutes.h"
#include "../Api/ApiService.h"

MainPageComponent::MainPageComponent():
#ifdef IN_RECEIVING_MODE
    webRTCAudioService(WebRTCAudioReceiverService()),
#else
    webRTCAudioService(WebRTCAudioSenderService()),
#endif
    webSocketService(WebSocketService(getWsRouteString(WsRoute::GetOngoingSession)))
{
    setSize(600, 400);
    addAndMakeVisible(appName);
    addAndMakeVisible(title);
    addAndMakeVisible(logoutButton);
    addAndMakeVisible(refreshButton);
    addAndMakeVisible(mainText);
    addAndMakeVisible(connectButton);
    addAndMakeVisible(RTCStateText);
    addAndMakeVisible(RTCSignalingStateText);
    addAndMakeVisible(RTCIceCandidateStateText);

    const auto userContext = AuthService::getInstance().getUserContext();
#ifdef MELO_PLUGIN_NAME
    appName.setText(juce::String::fromUTF8(MELO_PLUGIN_NAME), juce::dontSendNotification);
#else
    appName.setText(juce::String::fromUTF8("MeloVST"), juce::dontSendNotification);
#endif
    appName.setJustificationType(juce::Justification::centred);
    appName.setFont(30.0f);

    title.setText(juce::String::fromUTF8(("Bienvenue " + userContext->user.firstname).c_str()),
                  juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(30.0f);

    mainText.setText(juce::String::fromUTF8(("L'audio s'affichera ici")), juce::dontSendNotification);
    mainText.setJustificationType(juce::Justification::centred);

    RTCStateText.setText(juce::String::fromUTF8(("Vous n'êtes pas connecté avec l'artiste")),
                         juce::dontSendNotification);
    RTCStateText.setJustificationType(juce::Justification::centred);
    RTCIceCandidateStateText.setJustificationType(juce::Justification::centred);
    RTCSignalingStateText.setJustificationType(juce::Justification::centred);

    refreshButton.setButtonText(juce::String::fromUTF8("Rafraîchir"));
    refreshButton.onClick = [this] {
        fetchOngoingSession();
    };
    connectButton.setButtonText(juce::String::fromUTF8(("Se connecter avec l'artiste")));
    connectButton.onClick = [ meloWebRTCServerService = &webRTCAudioService] {
        if (meloWebRTCServerService->isConnecting()) {
            juce::Logger::outputDebugString("Already connecting...");
            meloWebRTCServerService->disconnect();
        } else if (meloWebRTCServerService->isConnected()) {
            juce::Logger::outputDebugString("Disconnecting from artist...");
            meloWebRTCServerService->disconnect();
        } else {
            juce::Logger::outputDebugString("Connecting from artist...");
            meloWebRTCServerService->setupConnection();
        }
    };

    logoutButton.setButtonText(juce::String::fromUTF8("Se déconnecter"));
    logoutButton.onClick = [] { onLogoutButtonClick(); };

    fetchOngoingSession();
    EventManager::getInstance().addListener(this);
}

void MainPageComponent::fetchOngoingSession() {
    if (const auto res = ApiService::makeGETRequest(ApiRoute::GetMyOngoingSessions); res.isNotEmpty()) {
        ongoingSessions = PopulatedSession::parseArrayFromJsonString(res);
        if (ongoingSessions.size() > 0) {
            currentOngoingSession = ongoingSessions[0];
            mainText.setText(
                "Vous avez une session en cours avec " + currentOngoingSession.reservedByArtist.user.userAlias,
                juce::dontSendNotification);
            mainText.setColour(juce::Label::textColourId, juce::Colours::green);
            EventManager::getInstance().notifyOngoingSessionChanged(OngoingSessionChangedEvent(currentOngoingSession));
            webSocketService.connectToServer();
        } else {
            mainText.setText("Vous n'avez pas de session en cours.", juce::dontSendNotification);
            mainText.setColour(juce::Label::textColourId, juce::Colours::red);
        }
    }
}

void MainPageComponent::onRTCStateChanged(const RTCStateChangeEvent &event) {
    juce::MessageManager::callAsync([this, event] {
        switch (event.state) {
            case rtc::PeerConnection::State::Connected: {
                connectButton.setButtonText(juce::String::fromUTF8("Déconnecter la connexion avec l'artiste"));
                RTCStateText.setText(juce::String::fromUTF8("Vous êtes connecté avec l'artiste."),
                                     juce::dontSendNotification);
                break;
            }
            case rtc::PeerConnection::State::Connecting: {
                connectButton.setButtonText(juce::String::fromUTF8(("Stopper la demande de connexion")));
                RTCStateText.setText("En cours de connexion avec l'artiste...", juce::dontSendNotification);
                break;
            }
            case rtc::PeerConnection::State::Closed:
            case rtc::PeerConnection::State::Failed: {
                connectButton.setVisible(true);
                connectButton.setButtonText(juce::String::fromUTF8(("Se connecter avec l'artiste")));
                RTCStateText.setText(juce::String::fromUTF8("Vous n'êtes pas connecté avec l'artiste"),
                                     juce::dontSendNotification);
                break;
            }
            default: {
                connectButton.setVisible(true);
                connectButton.setButtonText(juce::String::fromUTF8(("Se connecter avec l'artiste")));
                RTCStateText.setText(juce::String::fromUTF8("Vous n'êtes pas connecté avec l'artiste"),
                                     juce::dontSendNotification);
                break;
            }
        }

        RTCIceCandidateStateText.setText(webRTCAudioService.getIceCandidateStateLabel(), juce::dontSendNotification);
        RTCSignalingStateText.setText(webRTCAudioService.getSignalingStateLabel(),
                                      juce::dontSendNotification);
    });
}

MainPageComponent::~MainPageComponent() {
    logoutButton.onClick = nullptr;
    EventManager::getInstance().removeListener(this);
}

void MainPageComponent::resized() {
    juce::FlexBox stateFlexbox;
    stateFlexbox.flexDirection = juce::FlexBox::Direction::column;
    stateFlexbox.justifyContent = juce::FlexBox::JustifyContent::center;
    stateFlexbox.items.add(
        juce::FlexItem(RTCStateText).withFlex(0).withHeight(20).withMargin(juce::FlexItem::Margin(0, 0, 0, 0)));
    stateFlexbox.items.add(
        juce::FlexItem(RTCSignalingStateText).withFlex(0).withHeight(20).withMargin(
            juce::FlexItem::Margin(0, 0, 0, 0)));
    stateFlexbox.items.add(
        juce::FlexItem(RTCIceCandidateStateText).withFlex(0).withHeight(20).withMargin(
            juce::FlexItem::Margin(0, 0, 0, 0)));

    juce::FlexBox titleFlexbox;
    titleFlexbox.flexDirection = juce::FlexBox::Direction::column;
    titleFlexbox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    titleFlexbox.items.add(
        juce::FlexItem(logoutButton)
        .withFlex(0).withHeight(30).withWidth(70)
        .withMargin(juce::FlexItem::Margin(5, 5, 0, 0))
        .withAlignSelf(juce::FlexItem::AlignSelf::flexEnd)
    );
    titleFlexbox.items.add(
        juce::FlexItem(refreshButton)
        .withFlex(0).withHeight(30).withWidth(70)
        .withMargin(juce::FlexItem::Margin(5, 5, 0, 0))
        .withAlignSelf(juce::FlexItem::AlignSelf::flexEnd)
    );
    titleFlexbox.items.add(juce::FlexItem(appName).withFlex(1).withMargin(juce::FlexItem::Margin(10, 0, 0, 0)));
    titleFlexbox.items.add(juce::FlexItem(title).withFlex(1).withMargin(juce::FlexItem::Margin(10, 0, 0, 0)));
    titleFlexbox.items.add(juce::FlexItem(mainText).withFlex(1).withMargin(juce::FlexItem::Margin(0, 0, 0, 0)));

    juce::FlexBox pageFlexbox;
    pageFlexbox.flexDirection = juce::FlexBox::Direction::column;
    pageFlexbox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    pageFlexbox.alignContent = juce::FlexBox::AlignContent::center;

    pageFlexbox.items.add(juce::FlexItem(titleFlexbox).withFlex(1));
    pageFlexbox.items.add(juce::FlexItem(stateFlexbox).withFlex(1));
    pageFlexbox.items.add(
        juce::FlexItem(connectButton).withHeight(30).withWidth(200).withAlignSelf(juce::FlexItem::AlignSelf::center).
        withFlex(0).withMargin(juce::FlexItem::Margin(20, 0, 20, 0)));
    pageFlexbox.performLayout(getLocalBounds());
}

void MainPageComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::transparentWhite);
}

void MainPageComponent::onLogoutButtonClick() {
    AuthService::logout();
}
