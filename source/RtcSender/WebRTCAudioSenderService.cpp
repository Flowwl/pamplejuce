#include "WebRTCAudioSenderService.h"

#include "../Api/SocketRoutes.h"
#include "../Common/EventManager.h"
#include "../Debug/DebugRTPWrapper.h"
#include "../Config.h"

#include <rtc/rtc.hpp>

WebRTCAudioSenderService::WebRTCAudioSenderService() : WebRTCSenderConnexionHandler (WsRoute::GetOngoingSessionRTCInstru),
                                                       opusEncoder (Config::getInstance().opusSampleRate, Config::getInstance().dawNumChannels, Config::getInstance().latencyInMs, Config::getInstance().opusBitRate),
                                                       resampler (Config::getInstance().dawSampleRate, Config::getInstance().opusSampleRate,Config::getInstance().dawNumChannels)
{
}

WebRTCAudioSenderService::~WebRTCAudioSenderService()
{
    stopAudioThread();
}

void WebRTCAudioSenderService::onAudioBlockProcessedEvent(const AudioBlockProcessedEvent& event)
{
    if (event.data.empty())
    {
        return;
    }

    if (!threadRunning) {
        return;
    }

    // {
        // assert(audioQueueMutex.native_handle() != nullptr && "Mutex not initialized!");
        // std::lock_guard<std::mutex> lock(audioQueueMutex);
    // }
    audioEventQueue.push(std::make_shared<AudioBlockProcessedEvent>(event));
}

void WebRTCAudioSenderService::processingThreadFunction()
{
    std::vector<float> accumulationBuffer;
    accumulationBuffer.reserve(dawFrameSamplesWithAllChannels * 2);

    while (threadRunning.load())
    {
        assert(audioQueueMutex.native_handle() != nullptr && "Mutex not initialized!");
        std::unique_lock<std::mutex> lock(audioQueueMutex);

        if (!threadRunning.load()) break; // Vérification après `wait()`

        while (!audioEventQueue.empty())
        {
            auto eventPtr = audioEventQueue.top();
            audioEventQueue.pop();

            if (!eventPtr) continue; // Vérification de validité

            accumulationBuffer.insert(accumulationBuffer.end(), eventPtr->data.begin(), eventPtr->data.end());
        }

        lock.unlock(); // Libération du mutex avant traitement audio

        while (accumulationBuffer.size() >= static_cast<size_t>(dawFrameSamplesWithAllChannels))
        {
            sendAudioData(accumulationBuffer);
            accumulationBuffer.erase(accumulationBuffer.begin(), accumulationBuffer.begin() + dawFrameSamplesWithAllChannels);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void WebRTCAudioSenderService::startAudioThread()
{
    std::lock_guard<std::mutex> lock(threadMutex);
    if (!threadRunning)
    {
        dawFrameSamplesPerChannel = static_cast<int>(Config::getInstance().dawSampleRate * Config::getInstance().latencyInMs / 1000.0);
        dawFrameSamplesWithAllChannels = dawFrameSamplesPerChannel * Config::getInstance().dawNumChannels;

        threadRunning = true;
        encodingThread = std::thread(&WebRTCAudioSenderService::processingThreadFunction, this);
    }
}

void WebRTCAudioSenderService::stopAudioThread()
{
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        if (!threadRunning) return;
        threadRunning = false;
    }

    if (encodingThread.joinable())
    {
        encodingThread.join();
    }
}

void WebRTCAudioSenderService::sendAudioData (const std::vector<float>& accumulationBuffer)
{
    const std::vector<float> frameData (accumulationBuffer.begin(), accumulationBuffer.begin() + dawFrameSamplesWithAllChannels);
    const auto resampledData = resampler.resampleFromFloat (frameData);
    const auto resampledFrameSamplesPerChannel = resampledData.size() / Config::getInstance().dawNumChannels;
    std::vector<unsigned char> opusPacket = opusEncoder.encode_float(resampledData, resampledFrameSamplesPerChannel);
    timestamp += resampledFrameSamplesPerChannel;

    if (!opusPacket.empty() && audioTrack)
    {
        sendOpusPacket(opusPacket);
    }
}

void WebRTCAudioSenderService::sendOpusPacket (const std::vector<unsigned char>& opusPacket)
{
    try
    {
        if (!audioTrack)
        {
            juce::Logger::outputDebugString ("Error: audioTrack is null, skipping packet.");
            return;
        }
        auto rtpPacket = RTPWrapper::createRTPPacket (opusPacket, seqNum++, timestamp, ssrc);
        juce::Logger::outputDebugString ("Sending packet: seqNum=" + std::to_string (seqNum) + ", timestamp=" + std::to_string (timestamp) + ", size=" + std::to_string (rtpPacket.size()) + " bytes");
        audioTrack->send (reinterpret_cast<const std::byte*> (rtpPacket.data()), rtpPacket.size());
        rtpPacket.clear();
    } catch (const std::exception& e)
    {
        juce::Logger::outputDebugString ("Error sending audio data: " + std::string (e.what()));
    }
}

void WebRTCAudioSenderService::disconnect()
{
    stopAudioThread();
    WebRTCSenderConnexionHandler::disconnect();
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
