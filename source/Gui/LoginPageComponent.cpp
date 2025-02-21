//
// Created by Padoa on 08/01/2025.
//
#include "LoginPageComponent.h"

LoginPageComponent::LoginPageComponent() {
    setSize(600, 400);
    addAndMakeVisible(title);
    addAndMakeVisible(usernameLabel);
    addAndMakeVisible(usernameField);
    addAndMakeVisible(passwordLabel);
    addAndMakeVisible(passwordField);
    addAndMakeVisible(loginButton);
    addAndMakeVisible(enterInfosLabel);
    addAndMakeVisible(errorLabel);

    title.setText(juce::String::fromUTF8("Bienvenue sur le plugin VST Melo"), juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(30.0f);

    enterInfosLabel.setText(juce::String::fromUTF8("Veuillez entrer vos informations de connexion Melo"), juce::dontSendNotification);

    usernameLabel.setText(juce::String::fromUTF8("Email Melo"), juce::dontSendNotification);

    passwordLabel.setText(juce::String::fromUTF8("Mot de passe Melo"), juce::dontSendNotification);
    passwordField.setPasswordCharacter('*');

    errorLabel.setText(juce::String::fromUTF8("Nom d'utilisateur ou mot de passe incorrect"), juce::dontSendNotification);
    errorLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::red);
    errorLabel.setVisible(false);

    loginButton.setButtonText(juce::String::fromUTF8("Se connecter"));
    loginButton.onClick = [this] { onLoginButtonClick(); };
}

LoginPageComponent::~LoginPageComponent() = default;

void LoginPageComponent::resized() {
    juce::FlexBox loginInputFlexbox;
    loginInputFlexbox.flexDirection = juce::FlexBox::Direction::column;
    loginInputFlexbox.items.add(juce::FlexItem(usernameLabel).withFlex(0).withHeight(20));
    loginInputFlexbox.items.add(juce::FlexItem(usernameField).withHeight(30).withFlex(0).withMargin(juce::FlexItem::Margin(5, 0, 0, 5)));

    juce::FlexBox passwordInputFlexbox;
    passwordInputFlexbox.flexDirection = juce::FlexBox::Direction::column;
    passwordInputFlexbox.items.add(juce::FlexItem(passwordLabel).withFlex(0).withHeight(20));
    passwordInputFlexbox.items.add(juce::FlexItem(passwordField).withHeight(30).withFlex(0).withMargin(juce::FlexItem::Margin(5, 0, 0, 5)));

    juce::FlexBox formFlexbox;
    formFlexbox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    formFlexbox.alignContent = juce::FlexBox::AlignContent::flexStart;
    formFlexbox.flexDirection = juce::FlexBox::Direction::column;
    formFlexbox.items.add(juce::FlexItem(enterInfosLabel).withFlex(0).withHeight(30));
    formFlexbox.items.add(juce::FlexItem(loginInputFlexbox).withFlex(0).withHeight(50).withMargin(juce::FlexItem::Margin(20, 0, 0, 0)));
    formFlexbox.items.add(juce::FlexItem(passwordInputFlexbox).withFlex(0).withHeight(50).withMargin(juce::FlexItem::Margin(20, 0, 0, 0)));
    formFlexbox.items.add(juce::FlexItem(errorLabel).withFlex(0).withHeight(20));
    formFlexbox.items.add(juce::FlexItem(loginButton).withFlex(0).withHeight(40).withWidth(150).withMargin(juce::FlexItem::Margin(20, 0, 0, 5)).withAlignSelf(juce::FlexItem::AlignSelf::flexEnd));


    juce::FlexBox pageFlexbox;
    pageFlexbox.flexDirection = juce::FlexBox::Direction::column;
    pageFlexbox.justifyContent = juce::FlexBox::JustifyContent::center;
    pageFlexbox.alignContent = juce::FlexBox::AlignContent::center;
    pageFlexbox.items.add(juce::FlexItem(title).withFlex(0).withHeight(30).withMargin(juce::FlexItem::Margin(30, 0, 30, 0)));
    pageFlexbox.items.add(juce::FlexItem(formFlexbox).withFlex(1).withWidth(getWidth() * 0.5f).withAlignSelf(juce::FlexItem::AlignSelf::center));
    pageFlexbox.performLayout(getLocalBounds());

}

void LoginPageComponent::onLoginButtonClick() {
    if (const auto res = AuthService::getInstance().login(usernameField.getText(), passwordField.getText()); res.isEmpty()) {
        errorLabel.setVisible(true);
    }
}

void LoginPageComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::transparentWhite);
}