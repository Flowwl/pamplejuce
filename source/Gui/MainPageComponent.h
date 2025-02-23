//
// Created by Padoa on 08/01/2025.
//
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Api/AuthService.h"
#include "../Common/EventListener.h"
#include "../Api/WebSocketService.h"
#include "../Models/Session.h"
#include "../Debug/DebugAudioAppPlayer.h"
#include "OutputDevice.h"
#include "../RtcSender/WebRTCAudioSenderService.h"

class MainPageComponent final : public juce::Component, EventListener
{
public:
    explicit MainPageComponent();
    ~MainPageComponent() override;

    static void onLogoutButtonClick();
    void onRTCStateChanged(const RTCStateChangeEvent &event) override;
    void fetchOngoingSession();

    void resized() override;
    void paint(juce::Graphics &g) override;

private:
    juce::Label title, mainText, RTCStateText, RTCIceCandidateStateText, RTCSignalingStateText, appName;
    juce::TextButton logoutButton, connectButton, refreshButton;
    OutputDevice outputDevice;
    WebRTCAudioSenderService webRTCAudioService;
    // DebugAudioAppPlayer audioAppPlayer;
    WebSocketService webSocketService;
    juce::Array<PopulatedSession> ongoingSessions;
    PopulatedSession currentOngoingSession;
};