#include "WebRTCAudioSenderSendingHandler.h"

WebRTCAudioSenderSendingHandler::WebRTCAudioSenderSendingHandler(std::shared_ptr<rtc::Track> audioTrack) : opusEncoder (Config::getInstance().opusSampleRate, Config::getInstance().dawNumChannels, Config::getInstance().opusBitRate),
                                                                     resampler (Config::getInstance().dawSampleRate, Config::getInstance().opusSampleRate, Config::getInstance().dawNumChannels)
{
    this->audioTrack = audioTrack;
    refreshFrameSamples();
}

void WebRTCAudioSenderSendingHandler::refreshFrameSamples ()
{
    dawFrameSamplesPerChannel = Config::getInstance().dawSampleRate * Config::getInstance().latencyInMs / 1000;
    dawFrameSamplesWithAllChannels = dawFrameSamplesPerChannel * Config::getInstance().dawNumChannels;
}
void WebRTCAudioSenderSendingHandler::sendAudioData (const std::vector<float>& accumulationBuffer)
{
    if (accumulationBuffer.empty())
    {
        juce::Logger::outputDebugString ("Error: Trying to send empty buffer.");
        return;
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

void WebRTCAudioSenderSendingHandler::sendOpusPacket (const std::vector<unsigned char>& opusPacket)
{
    try
    {
        if (!audioTrack)
        {
            juce::Logger::outputDebugString ("Error: audioTrack is null, skipping packet.");
            return;
        }

        auto rtpPacket = RTPWrapper::createRTPPacket (opusPacket, seqNum++, timestamp, Config::getInstance().ssrc);

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