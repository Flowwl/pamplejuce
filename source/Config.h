#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Config
{
    // URL de l'API
    static const juce::String apiUrl = "http://localhost:5055/api";
    static const juce::String websocketUrl = "ws://localhost:5055";
    // static const juce::String apiUrl = "https://staging.studio-melo.com/api";
    // static const juce::String websocketUrl = "wss://staging.studio-melo.com";

    // Couleurs
    static const auto primaryColor = juce::Colour(0xFF1A1A1A);
    static const juce::Colour gray900 = juce::Colour::fromFloatRGBA(12, 17, 29, 1.0f);
    static const juce::Colour gray90080 = juce::Colour::fromFloatRGBA(12, 17, 29, 0.8f);

}
