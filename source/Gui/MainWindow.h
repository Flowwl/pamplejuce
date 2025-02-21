//
// Created by Padoa on 08/01/2025.
//
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MainPageComponent.h"
#include "../Common/EventListener.h"

class MainWindow final : public juce::Component, public EventListener
{
public:
    explicit MainWindow (const juce::String& name);
    ~MainWindow() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void onLoginEvent(const LoginEvent &event) override;
    void onLogoutEvent(const LogoutEvent &event) override;

private:
   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)

    std::unique_ptr<Component> currentPage;
    void navigateToLoginPage();
    void navigateToMainPage();
};