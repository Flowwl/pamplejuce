#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class Config
{
public:
    static Config& getInstance() {
        static Config instance;
        return instance;
    }

    // URL de l'API
    const juce::String apiUrl = "http://localhost:5055/api";
    const juce::String websocketUrl = "ws://localhost:5055";
    // static const juce::String apiUrl = "https://staging.studio-melo.com/api";
    // static const juce::String websocketUrl = "wss://staging.studio-melo.com";

    // Couleurs
    const juce::Colour primaryColor = juce::Colour(0xFF1A1A1A);
    const juce::Colour gray900 = juce::Colour::fromFloatRGBA(12, 17, 29, 1.0f);
    const juce::Colour gray90080 = juce::Colour::fromFloatRGBA(12, 17, 29, 0.8f);


    // Audio
    const u_int32_t opusSampleRate = 48000;
    const int latencyInMs = 20;
    const u_int32_t opusBitRate = 96000;
    int dawBitDepth = 16;
    u_int32_t dawSampleRate = 44100;
    int dawBlockSize = 256;
    int dawNumChannels = 2;
    int useInBandFec = 1;
    int codecPayloadType = 111;
    std::string rtcStunServer = "stun:stun.l.google.com:19302";
    int ssrc = 12345;
    std::string cname = "CNAME";

    void setDawBlockSize (const int _blockSize) { dawBlockSize = _blockSize; }
    void setDawNumChannels (const int _numChannels) { dawNumChannels = _numChannels; }
    void setDawSampleRate (const int _sampleRate) { dawSampleRate = _sampleRate; }
    void setDawBitDepth (const int _bitDepth) { dawBitDepth = _bitDepth; }
};
