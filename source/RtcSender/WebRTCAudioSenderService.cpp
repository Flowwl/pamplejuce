#include "WebRTCAudioSenderService.h"

#include "../Api/SocketRoutes.h"
#include "../Common/EventManager.h"
#include "../Config.h"
#include "../Debug/DebugRTPWrapper.h"

#include <rtc/rtc.hpp>

WebRTCAudioSenderService::WebRTCAudioSenderService() : WebRTCSenderConnexionHandler (WsRoute::GetOngoingSessionRTCInstru), sendingHandler(this->audioTrack)
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
    // juce::Logger::outputDebugString ("Added audio block, nb packets inside queue " + juce::String (audioEventQueue.size()));
}

void WebRTCAudioSenderService::processingThreadFunction()
{
    std::vector<float> accumulationBuffer;
    accumulationBuffer.reserve (sendingHandler.getDawFrameSamplesWithAllChannels() * 2);

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
                // juce::Logger::outputDebugString ("Added " + juce::String (eventPtr->data.size()) + " samples to buffer.");
            }
        }
        lock.unlock();

        while (accumulationBuffer.size() >= targetFrameSamples)
        {
            sendingHandler.sendAudioData (accumulationBuffer);
            accumulationBuffer.erase (accumulationBuffer.begin(), accumulationBuffer.begin() + targetFrameSamples);
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
        sendingHandler.refreshFrameSamples ();
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
