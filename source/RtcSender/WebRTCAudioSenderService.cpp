#include "WebRTCAudioSenderService.h"

#include "../Api/SocketRoutes.h"
#include "../Common/EventManager.h"
#include "../Config.h"
#include "../Debug/DebugRTPWrapper.h"

#include <rtc/rtc.hpp>

WebRTCAudioSenderService::WebRTCAudioSenderService() : WebRTCSenderConnexionHandler (WsRoute::GetOngoingSessionRTCInstru)
{
}

WebRTCAudioSenderService::~WebRTCAudioSenderService()
{
    stopAudioThread();
}

void WebRTCAudioSenderService::onAudioBlockProcessedEvent(const AudioBlockProcessedEvent& event)
{
    if (event.data.empty() || !threadRunning.load()) {
        return;
    }

    auto eventPtr = std::make_shared<AudioBlockProcessedEvent>(event);
    {
        std::unique_lock<std::mutex> lock(audioQueueMutex);
        audioEventQueue.push(eventPtr);
    }
    cv.notify_one(); // Réveiller le thread
}

void WebRTCAudioSenderService::processingThreadFunction()
{
    std::vector<float> accumulationBuffer;
    accumulationBuffer.reserve(sendingHandler.getDawFrameSamplesWithAllChannels() * 2);

    while (threadRunning.load()) {
        std::unique_lock<std::mutex> lockCv(cvMutex);
        cv.wait_for(lockCv, std::chrono::milliseconds(Config::getInstance().latencyInMs), [&]() { return threadShouldExit.load(); });

        if (threadShouldExit.load()) {
            break; // Sortie propre
        }

        lockCv.unlock(); // Libérer avant d'appeler processAudio
        processAudio(accumulationBuffer);
    }

    juce::Logger::outputDebugString("Processing thread exited cleanly.");
}


void WebRTCAudioSenderService::processAudio(std::vector<float>& accumulationBuffer)
{
    std::vector<std::shared_ptr<AudioBlockProcessedEvent>> localQueue;

    {
        std::unique_lock<std::mutex> lock(audioQueueMutex);
        while (!audioEventQueue.empty()) {
            localQueue.push_back(audioEventQueue.top());
            audioEventQueue.pop();
        }
    } // On libère le mutex ici, une fois la copie faite

    for (const auto& eventPtr : localQueue) {
        if (eventPtr) {
            accumulationBuffer.insert(accumulationBuffer.end(), eventPtr->data.begin(), eventPtr->data.end());
        }
    }

    while (accumulationBuffer.size() >= sendingHandler.getDawFrameSamplesWithAllChannels()) {
        sendingHandler.sendAudioData(accumulationBuffer);
        accumulationBuffer.erase(accumulationBuffer.begin(), accumulationBuffer.begin() + sendingHandler.getDawFrameSamplesWithAllChannels());
    }
}


void WebRTCAudioSenderService::startAudioThread()
{
    std::lock_guard<std::mutex> lock (threadMutex);
    if (!threadRunning.load())
    {
        sendingHandler.refreshFrameSamples();
        threadShouldExit = false;
        threadRunning = true;
        encodingThread = std::thread (&WebRTCAudioSenderService::processingThreadFunction, this);
    }
}

void WebRTCAudioSenderService::stopAudioThread()
{
    {
        std::lock_guard<std::mutex> lock(threadMutex);
        threadRunning = false;
        threadShouldExit = true;
    }

    cv.notify_all(); // Réveiller le thread immédiatement

    if (encodingThread.joinable()) {
        encodingThread.join();
        juce::Logger::outputDebugString("Audio thread stopped cleanly.");
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
        sendingHandler.setAudioTrack (audioTrack);
        startAudioThread();
    }
    else
    {
        stopAudioThread();
    }
}
