//
// Created by Padoa on 08/01/2025.
//
#include "MainWindow.h"
#include "LoginPageComponent.h"
#include "../Common/EventManager.h"
#include "../Common/JuceLocalStorage.h"

MainWindow::MainWindow(const juce::String& name): Component(name)
{
    if (const auto accessToken = JuceLocalStorage::getInstance().loadValue("access_token"); accessToken.isEmpty()) {
        navigateToLoginPage();
    }
    else {
        auto autoContext = AuthService::getInstance().fetchUserContext();
        if (autoContext.has_value()) {
            navigateToMainPage();
        }
        else {
            navigateToLoginPage();
        }
    }

    Component::setVisible (true);
    EventManager::getInstance().addListener(this);
}

MainWindow::~MainWindow() {
    EventManager::getInstance().removeListener(this);
}

void MainWindow::paint(juce::Graphics& g)
{
    g.fillAll (juce::Colours::transparentBlack);
}

void MainWindow::resized()
{
    const auto area = getLocalBounds();

    if (currentPage != nullptr)
        currentPage->setBounds(area);
}

void MainWindow::onLoginEvent(const LoginEvent& event) {
    navigateToMainPage();
}

void MainWindow::onLogoutEvent(const LogoutEvent& event) {
    navigateToLoginPage();
}

void MainWindow::navigateToLoginPage()
{
    currentPage = std::make_unique<LoginPageComponent>();
    addAndMakeVisible(currentPage.get());
    resized();
}

void MainWindow::navigateToMainPage()
{
    currentPage = std::make_unique<MainPageComponent>();
    addAndMakeVisible(currentPage.get());
    resized();
}
