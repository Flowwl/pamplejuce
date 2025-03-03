#include "MainAudioProcessor.h"
#include "./Common/EventManager.h"
#include "Config.h"
#include "MainApplication.h"
#include <chrono>

//==============================================================================
MainAudioProcessor::MainAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
              .withInput ("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
#ifdef IN_RECEIVING_MODE
// , audioPacketQueue(Config::getInstance().dawSampleRate * Config::getInstance().dawNumChannels)
#endif
{
    EventManager::getInstance().addListener (this);
}

MainAudioProcessor::~MainAudioProcessor()
{
    EventManager::getInstance().removeListener (this);
};

const juce::String MainAudioProcessor::getProgramName (const int index)
{
    return "MeloVST";
}

//==============================================================================
const juce::String MainAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MainAudioProcessor::acceptsMidi() const
{
    return false;
}

bool MainAudioProcessor::producesMidi() const
{
    return true;
}

bool MainAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double MainAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MainAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int MainAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MainAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

void MainAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void MainAudioProcessor::prepareToPlay (const double sampleRate, const int samplesPerBlock)
{
    Config::getInstance().setDawSampleRate (sampleRate);
    Config::getInstance().setDawBlockSize (samplesPerBlock);
    Config::getInstance().setDawNumChannels (getMainBusNumOutputChannels());

    juce::Logger::outputDebugString ("Sample rate: " + std::to_string (sampleRate));
    juce::Logger::outputDebugString ("Block Size: " + std::to_string (samplesPerBlock));
    juce::Logger::outputDebugString ("Num Channels: " + std::to_string (getMainBusNumOutputChannels()));
    juce::Logger::outputDebugString ("Bit depth: " + std::to_string (Config::getInstance().dawBitDepth));
    juce::Logger::outputDebugString (
        "Opus Sample rate: " + std::to_string (Config::getInstance().opusSampleRate));
    juce::Logger::outputDebugString ("Latency: " + std::to_string (Config::getInstance().latencyInMs));
    juce::Logger::outputDebugString ("Opus Bit rate: " + std::to_string (Config::getInstance().opusBitRate));
}

void MainAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool MainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return true;
}

#ifdef IN_RECEIVING_MODE

void MainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    buffer.clear();

    int samplesWritten = 0;

    while (samplesWritten < numSamples)
    {
        {
            juce::ScopedLock sl (lock);

            if (currentBlock.empty())
            {
                if (audioQueue.empty())
                {
                    break;
                }
                else
                {
                    currentBlock = std::move (audioQueue.front());
                    audioQueue.pop_front();
                    currentSampleIndex = 0;
                }
            }
        }

        int samplesRemainingInBlock = static_cast<int> (currentBlock.size()) - currentSampleIndex;
        int samplesToCopy = std::min (numSamples - samplesWritten, samplesRemainingInBlock);
        juce::Logger::outputDebugString ("Samples to copy: " + std::to_string (samplesToCopy));
        juce::Logger::outputDebugString ("nb channels: " + std::to_string (numChannels));
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto writePointer = buffer.getWritePointer (channel, samplesWritten);
            for (int sample = 0; sample < samplesToCopy; ++sample)
            {
                writePointer[sample] = currentBlock[currentSampleIndex + sample];
            }
        }

        samplesWritten += samplesToCopy;
        {
            juce::ScopedLock sl (lock);
            currentSampleIndex += samplesToCopy;

            if (currentSampleIndex >= static_cast<int> (currentBlock.size()))
            {
                currentBlock.clear();
                currentSampleIndex = 0;
            }
        }
    }
}

void MainAudioProcessor::onAudioBlockReceivedDecoded (const AudioBlockReceivedDecodedEvent& event)
{
    juce::ScopedLock sl (lock);
    audioQueue.push_back (event.data);
}

#else
void MainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
    {
        return; // Rien à traiter
    }

    // Optimisation mémoire : éviter les réallocations fréquentes
    std::vector<float> pcmData;
    pcmData.reserve (static_cast<size_t> (numChannels * numSamples));

    // Copier les échantillons dans un format interleaved (L, R, L, R...)
    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            pcmData.push_back (buffer.getReadPointer (channel)[sample]);
        }
    }

    if (pcmData.empty())
        return; // Vérification plus explicite

    // Envoyer l'événement avec std::move pour éviter les copies inutiles
    EventManager::getInstance().notifyAudioBlockProcessed (AudioBlockProcessedEvent {
        std::move (pcmData), // Transfert des données
        numChannels,
        numSamples,
        getSampleRate(),
        juce::Time::currentTimeMillis() });
}
#endif

//==============================================================================
bool MainAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MainAudioProcessor::createEditor()
{
    return new MainApplication (*this);
}

//==============================================================================
void MainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void MainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MainAudioProcessor();
}
