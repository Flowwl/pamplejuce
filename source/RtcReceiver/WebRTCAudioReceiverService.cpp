#include "WebRTCAudioReceiverService.h"
#include "../Debug/DebugRTPWrapper.h"
#include "../AudioSettings.h"
#include "../Common/EventManager.h"
#include <rtc/rtc.hpp>
#include "../Api/SocketRoutes.h"
#include "../Utils/VectorUtils.h"
int error = 0;

WebRTCAudioReceiverService::WebRTCAudioReceiverService(): WebRTCReceiverConnexionHandler(
                                                              WsRoute::GetOngoingSessionRTCVoice),
                                                          opusCodec(AudioSettings::getInstance().getOpusSampleRate(),
                                                                    AudioSettings::getInstance().getNumChannels(),
                                                                    AudioSettings::getInstance().getLatency(),
                                                                    AudioSettings::getInstance().getOpusBitRate()),
                                                          resampler(AudioSettings::getInstance().getSampleRate(),
                                                                    AudioSettings::getInstance().getOpusSampleRate(),
                                                                    AudioSettings::getInstance().getNumChannels()),
                                                          decoder(opus_decoder_create(48000, 1, &error))
{
    if (error != OPUS_OK) {
        std::cerr << "Échec de l'initialisation du décodeur Opus: " << opus_strerror(error) << std::endl;
    }
}

WebRTCAudioReceiverService::~WebRTCAudioReceiverService() {
}

void WebRTCAudioReceiverService::onAudioBlockReceived(const AudioBlockReceivedEvent &event){
    rtc::message_variant audioBlock = event.data;
    uint64_t timestamp = event.timestamp;

    if (!std::holds_alternative<rtc::binary>(audioBlock))
        return;

    const rtc::binary& msg = std::get<rtc::binary>(audioBlock);
    size_t rtpHeaderSize = 28; // Vérifie que c'est bien la bonne taille pour ton cas
    if (msg.size() < rtpHeaderSize)
        return; // Erreur : paquet trop petit

    rtc::binary encodedData(msg.begin() + rtpHeaderSize, msg.end());
    std::vector<unsigned char> encoded(encodedData.size());
    std::transform(encodedData.begin(), encodedData.end(), encoded.begin(), [](std::byte b) {
        return static_cast<unsigned char>(b);
    });

    auto packetFrameSize = opus_packet_get_samples_per_frame(encoded.data(), 48000);
    auto nbChannels = opus_packet_get_nb_channels(encoded.data());
    juce::ignoreUnused(packetFrameSize, nbChannels);

    auto nbSamples = opus_packet_get_nb_samples(encoded.data(), encoded.size(), 48000);
    juce::Logger::outputDebugString("Audio block received with " + std::to_string(encoded.size()) + " bytes");

    std::vector<float> pcmBuffer(nbSamples, 0.0f);
    int frameSize = opus_decode_float(decoder, encoded.data(), encoded.size(), pcmBuffer.data(), nbSamples, 0);
    if (frameSize < 0) {
        std::cerr << "Erreur lors du décodage Opus : " << opus_strerror(frameSize) << std::endl;
        return;
    }

    pcmBuffer.resize(frameSize);
    EventManager::getInstance().notifyOnAudioBlockReceivedDecoded(AudioBlockReceivedDecodedEvent{pcmBuffer, timestamp});
}
