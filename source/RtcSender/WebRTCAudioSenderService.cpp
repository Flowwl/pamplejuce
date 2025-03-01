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
        startAudioThread();
    }
}

void WebRTCAudioSenderService::processingThreadFunction()
{
    // 48000 * 20/1000 = 960
    const int dawFrameSamplesPerChannel = static_cast<int>(AudioSettings::getInstance().getSampleRate() * AudioSettings::getInstance().getLatency() / 1000.0);
    // 960 * 2 = 1920
    const int dawFrameSamplesWithAllChannels = dawFrameSamplesPerChannel * AudioSettings::getInstance().getNumChannels();

    std::vector<float> accumulationBuffer;
    accumulationBuffer.reserve(dawFrameSamplesWithAllChannels * 2);

    while (threadRunning)
    {
        {
            const juce::ScopedLock sl(dequeLock);
            while (!audioEventQueue.empty())
            {
                auto eventPtr = audioEventQueue.top();
                audioEventQueue.pop();
                accumulationBuffer.reserve(accumulationBuffer.size() + eventPtr->data.size());
                accumulationBuffer.insert(accumulationBuffer.end(), eventPtr->data.begin(), eventPtr->data.end());
            }
        }

        while (accumulationBuffer.size() >= static_cast<size_t>(dawFrameSamplesWithAllChannels))
        {
            std::vector<float> frameData(accumulationBuffer.begin(), accumulationBuffer.begin() + dawFrameSamplesWithAllChannels);

            // EventManager::getInstance().notifyOnAudioBlockSent(AudioBlockSentEvent{ frameData, packetTimestamp });
            auto resampledData = resampler.resampleFromFloat(frameData);
            auto resampledFrameSamplesPerChannel = resampledData.size() / AudioSettings::getInstance().getNumChannels();
            std::vector<unsigned char> opusPacket = opusEncoder.encode_float(resampledData, resampledFrameSamplesPerChannel);
            timestamp += resampledFrameSamplesPerChannel;
            resampledData.clear();
            if (!opusPacket.empty() && audioTrack)
            {
                sendOpusPacket(opusPacket);
            }
            opusPacket.clear();
            if (accumulationBuffer.size() >= static_cast<size_t>(dawFrameSamplesWithAllChannels)) {
                accumulationBuffer.erase(accumulationBuffer.begin(), accumulationBuffer.begin() + dawFrameSamplesWithAllChannels);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

void WebRTCAudioSenderService::sendOpusPacket (const std::vector<unsigned char>& opusPacket)
{
    try
    {
        auto rtpPacket = RTPWrapper::createRTPPacket (opusPacket, seqNum++, timestamp, ssrc);
        juce::Logger::outputDebugString ("Sending packet: seqNum=" + std::to_string (seqNum) + ", timestamp=" + std::to_string (timestamp) + ", size=" + std::to_string (rtpPacket.size()) + " bytes");
        audioTrack->send (reinterpret_cast<const std::byte*> (rtpPacket.data()), rtpPacket.size());
        rtpPacket.clear();
    } catch (const std::exception& e)
    {
        juce::Logger::outputDebugString ("Error sending audio data: " + std::string (e.what()));
    }
}

void WebRTCAudioSenderService::startAudioThread()
{
    if (!threadRunning && peerConnection->state() == rtc::PeerConnection::State::Connected)
    {
        threadRunning = true;
        encodingThread = std::thread (&WebRTCAudioSenderService::processingThreadFunction, this);
    }
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
