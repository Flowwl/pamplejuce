#include "WebRTCAudioSenderService.h"
#include "../Api/SocketRoutes.h"
#include "../AudioSettings.h"
#include "../Common/EventManager.h"
#include "../Debug/DebugRTPWrapper.h"

#include <rtc/rtc.hpp>

WebRTCAudioSenderService::WebRTCAudioSenderService() : WebRTCSenderConnexionHandler (WsRoute::GetOngoingSessionRTCInstru),
                                                       opusEncoder (AudioSettings::getInstance().getOpusSampleRate(),
                                                           AudioSettings::getInstance().getNumChannels(),
                                                           AudioSettings::getInstance().getLatency(),
                                                           AudioSettings::getInstance().getOpusBitRate()),
                                                       resampler (AudioSettings::getInstance().getSampleRate(),
                                                           AudioSettings::getInstance().getOpusSampleRate(),
                                                           AudioSettings::getInstance().getNumChannels())
{
}

WebRTCAudioSenderService::~WebRTCAudioSenderService()
{
    stopAudioThread();
}

void WebRTCAudioSenderService::onAudioBlockProcessedEvent (const AudioBlockProcessedEvent& event)
{
    if (event.data.empty())
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock (audioQueueMutex);
        audioEventQueue.push (std::make_shared<AudioBlockProcessedEvent> (event));
    }
}

void WebRTCAudioSenderService::processingThreadFunction()
{
    // Calcul du nombre d'échantillons par canal pour une trame de 20ms
    const int frameSamples = static_cast<int>(
        AudioSettings::getInstance().getSampleRate() * 10 / 1000.0); // 48000 * 20/1000 = 960
    const int totalFrameSamples = frameSamples * AudioSettings::getInstance().getNumChannels(); // ex: 960 * 2 = 1920

    // Buffer temporaire pour accumuler les samples (interleaved)
    std::vector<float> accumulationBuffer;
    accumulationBuffer.reserve(totalFrameSamples * 2); // pré-allocation (optionnelle)

    while (threadRunning)
    {
        // Récupérer les événements audio dans l'ordre
        {
            const juce::ScopedLock sl(dequeLock);
            while (!audioEventQueue.empty())
            {
                auto eventPtr = audioEventQueue.top();
                audioEventQueue.pop();

                // On ajoute les samples du bloc dans le buffer d'accumulation
                accumulationBuffer.insert(accumulationBuffer.end(), eventPtr->data.begin(), eventPtr->data.end());
            }
        }

        // Tant que nous avons suffisamment de samples pour constituer une trame
        while (accumulationBuffer.size() >= static_cast<size_t>(totalFrameSamples))
        {
            // Extraire exactement totalFrameSamples depuis le début
            std::vector<float> frameData(accumulationBuffer.begin(),
                                         accumulationBuffer.begin() + totalFrameSamples);
            // Notifier l'envoi (optionnel)
            // EventManager::getInstance().notifyOnAudioBlockSent(AudioBlockSentEvent{ frameData, packetTimestamp });

            // Encoder la trame en packet Opus
            std::vector<unsigned char> opusPacket = opusEncoder.encode_float(frameData, frameSamples);
            if (!opusPacket.empty() && audioTrack)
            {
                sendOpusPacket(opusPacket, timestamp, frameSamples);
            }

            // Supprimer les samples traités du buffer d'accumulation
            accumulationBuffer.erase(accumulationBuffer.begin(),
                                     accumulationBuffer.begin() + totalFrameSamples);
        }
        // Attente si pas assez de données accumulées
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void WebRTCAudioSenderService::sendOpusPacket (const std::vector<unsigned char>& opusPacket, uint32_t packetTimestamp, int frameSamples)
{
    try
    {
        timestamp += frameSamples;
        auto rtpPacket = RTPWrapper::createRTPPacket (opusPacket, seqNum++, timestamp, ssrc);
        juce::Logger::outputDebugString ("Sending packet: seqNum=" + std::to_string (seqNum) + ", timestamp=" + std::to_string (packetTimestamp) + "vs inTimestamp=" + std::to_string (timestamp) + ", size=" + std::to_string (rtpPacket.size()) + " bytes");
        audioTrack->send (reinterpret_cast<const std::byte*> (rtpPacket.data()), rtpPacket.size());
    } catch (const std::exception& e)
    {
        juce::Logger::outputDebugString ("Error sending audio data: " + std::string (e.what()));
    }
}

void WebRTCAudioSenderService::startAudioThread()
{
    threadRunning = true;
    encodingThread = std::thread (&WebRTCAudioSenderService::processingThreadFunction, this);
}

void WebRTCAudioSenderService::stopAudioThread()
{
    threadRunning = false;
    if (encodingThread.joinable())
        encodingThread.join();
}

void WebRTCAudioSenderService::onRTCStateChanged (const RTCStateChangeEvent& event)
{
    if (event.state == rtc::PeerConnection::State::Connected && !threadRunning)
    {
        startAudioThread();
    }
    else
    {
        stopAudioThread();
    }
}
