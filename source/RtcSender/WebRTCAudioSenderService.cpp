#include "WebRTCAudioSenderService.h"

#include "../Api/SocketRoutes.h"
#include "../Common/EventManager.h"
#include "../Config.h"
#include "../Debug/DebugRTPWrapper.h"

#include <rtc/rtc.hpp>

WebRTCAudioSenderService::WebRTCAudioSenderService() : WebRTCSenderConnexionHandler (WsRoute::GetOngoingSessionRTCInstru),
                                                       opusEncoder (Config::getInstance().opusSampleRate, Config::getInstance().dawNumChannels, Config::getInstance().latencyInMs, Config::getInstance().opusBitRate),
                                                       resampler (Config::getInstance().dawSampleRate, Config::getInstance().opusSampleRate, Config::getInstance().dawNumChannels)
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

    if (!threadRunning)
    {
        return;
    }

    // {
    // assert(audioQueueMutex.native_handle() != nullptr && "Mutex not initialized!");
    // std::lock_guard<std::mutex> lock(audioQueueMutex);
    // }
    audioEventQueue.push (std::make_shared<AudioBlockProcessedEvent> (event));
    juce::Logger::outputDebugString ("Added audio block to queue of size " + juce::String (audioEventQueue.size()));
}

void WebRTCAudioSenderService::processingThreadFunction()
{
    std::vector<float> accumulationBuffer;
    accumulationBuffer.reserve (dawFrameSamplesWithAllChannels * 2);

    const int targetFrameSamples = static_cast<int> (Config::getInstance().dawSampleRate * 20 / 1000.0) * Config::getInstance().dawNumChannels;

    juce::int64 lastSendTime = juce::Time::currentTimeMillis();

    while (threadRunning.load())
    {
        juce::int64 processingStartTime = juce::Time::currentTimeMillis();

        std::unique_lock<std::mutex> lock (audioQueueMutex);
        while (!audioEventQueue.empty())
        {
            auto eventPtr = audioEventQueue.top();
            audioEventQueue.pop();

            if (eventPtr)
            {
                accumulationBuffer.insert (accumulationBuffer.end(), eventPtr->data.begin(), eventPtr->data.end());
                juce::Logger::outputDebugString ("Added " + juce::String (eventPtr->data.size()) + " samples to buffer.");
            }
        }
        lock.unlock();

        while (accumulationBuffer.size() >= targetFrameSamples)
        {
            sendAudioData (accumulationBuffer);

            // Adapter la taille de l'effacement en fonction du sample rate
            int eraseSize = targetFrameSamples;
            if (Config::getInstance().dawSampleRate > 48000)
            {
                eraseSize = targetFrameSamples / 2; // Réduit l'accumulation excessive
            }

            accumulationBuffer.erase (accumulationBuffer.begin(), accumulationBuffer.begin() + eraseSize);
        }

        juce::int64 processingEndTime = juce::Time::currentTimeMillis();
        juce::int64 processingDuration = processingEndTime - processingStartTime;
        juce::int64 waitTime = 20 - processingDuration;

        if (waitTime > 0)
        {
            std::this_thread::sleep_for (std::chrono::milliseconds (waitTime));
        }

        lastSendTime = juce::Time::currentTimeMillis();
    }
}

void WebRTCAudioSenderService::startAudioThread()
{
    std::lock_guard<std::mutex> lock (threadMutex);
    if (!threadRunning)
    {
        dawFrameSamplesPerChannel = static_cast<int> (Config::getInstance().dawSampleRate * Config::getInstance().latencyInMs / 1000.0);
        dawFrameSamplesWithAllChannels = dawFrameSamplesPerChannel * Config::getInstance().dawNumChannels;

        threadRunning = true;
        encodingThread = std::thread (&WebRTCAudioSenderService::processingThreadFunction, this);
    }
}

void WebRTCAudioSenderService::stopAudioThread()
{
    {
        std::lock_guard<std::mutex> lock (threadMutex);
        threadRunning = false;
    }

    if (encodingThread.joinable())
    {
        encodingThread.join();
    }
}

void WebRTCAudioSenderService::sendAudioData (const std::vector<float>& accumulationBuffer)
{
    if (accumulationBuffer.empty())
    {
        juce::Logger::outputDebugString ("Error: Trying to send empty buffer.");
        return; // Évite un crash potentiel
    }

    const std::vector<float> frameData (accumulationBuffer.begin(), accumulationBuffer.begin() + dawFrameSamplesWithAllChannels);
    const auto resampledData = resampler.resampleFromFloat (frameData);

    if (resampledData.empty())
    {
        juce::Logger::outputDebugString ("Error: Resampled data is empty.");
        return;
    }

    const auto resampledFrameSamplesPerChannel = resampledData.size() / Config::getInstance().dawNumChannels;
    std::vector<unsigned char> opusPacket = opusEncoder.encode_float (resampledData, resampledFrameSamplesPerChannel);
    timestamp += resampledFrameSamplesPerChannel;

    if (opusPacket.empty())
    {
        juce::Logger::outputDebugString ("Warning: Opus packet is empty, skipping transmission.");
        return;
    }

    if (audioTrack)
    {
        sendOpusPacket (opusPacket);
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

        juce::int64 now = juce::Time::currentTimeMillis(); // Ajout de la variable `now`
        juce::Logger::outputDebugString ("Sending packet: seqNum=" + std::to_string (seqNum) + ", timestamp=" + std::to_string (timestamp) + ", size=" + std::to_string (rtpPacket.size()) + " bytes");
        juce::Logger::outputDebugString ("Packet sent - Delta time: " + std::to_string (now - lastSentToRTCTime) + " ms");

        audioTrack->send (reinterpret_cast<const std::byte*> (rtpPacket.data()), rtpPacket.size());
        rtpPacket.clear();

        lastSentToRTCTime = now;
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
