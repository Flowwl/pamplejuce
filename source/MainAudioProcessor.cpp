#include "MainAudioProcessor.h"
#include "MainApplication.h"
#include "AudioSettings.h"
#include "Common/EventManager.h"

//==============================================================================
MainAudioProcessor::MainAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#ifdef IN_RECEIVING_MODE
// , audioPacketQueue(AudioSettings::getInstance().getSampleRate() * AudioSettings::getInstance().getNumChannels())
#endif
{
    EventManager::getInstance().addListener(this);
}

MainAudioProcessor::~MainAudioProcessor() {
    EventManager::getInstance().removeListener(this);
};

const juce::String MainAudioProcessor::getProgramName(const int index) {
#ifdef MELO_PLUGIN_NAME
    return MELO_PLUGIN_NAME;
#else
    return "MeloVST";
#endif
}


//==============================================================================
const juce::String MainAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool MainAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool MainAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool MainAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double MainAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int MainAudioProcessor::getNumPrograms() {
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int MainAudioProcessor::getCurrentProgram() {
    return 0;
}

void MainAudioProcessor::setCurrentProgram(int index) {
    juce::ignoreUnused(index);
}

void MainAudioProcessor::changeProgramName(int index, const juce::String &newName) {
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void MainAudioProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock) {
    AudioSettings::getInstance().setSampleRate(sampleRate);
    AudioSettings::getInstance().setBlockSize(samplesPerBlock);
    AudioSettings::getInstance().setNumChannels(getMainBusNumOutputChannels());
    AudioSettings::getInstance().setBitDepth(16);
    AudioSettings::getInstance().setOpusSampleRate(48000);
    AudioSettings::getInstance().setLatency(20); // 20 ms
    AudioSettings::getInstance().setOpusBitRate(96000);

    juce::Logger::outputDebugString("Sample rate: " + std::to_string(sampleRate));
    juce::Logger::outputDebugString("Block Size: " + std::to_string(samplesPerBlock));
    juce::Logger::outputDebugString("Num Channels: " + std::to_string(getMainBusNumOutputChannels()));
    juce::Logger::outputDebugString("Bit depth: " + std::to_string(AudioSettings::getInstance().getBitDepth()));
    juce::Logger::outputDebugString(
        "Opus Sample rate: " + std::to_string(AudioSettings::getInstance().getOpusSampleRate()));
    juce::Logger::outputDebugString("Latency: " + std::to_string(AudioSettings::getInstance().getLatency()));
    juce::Logger::outputDebugString("Opus Bit rate: " + std::to_string(AudioSettings::getInstance().getOpusBitRate()));
}

void MainAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool MainAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

#ifdef IN_RECEIVING_MODE


void MainAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                      juce::MidiBuffer &midiMessages) {
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    buffer.clear();

    int samplesWritten = 0;

    while (samplesWritten < numSamples) {
        {
            juce::ScopedLock sl(lock);

            if (currentBlock.empty()) {
                if (audioQueue.empty()) {
                    break;
                } else {
                    currentBlock = std::move(audioQueue.front());
                    audioQueue.pop_front();
                    currentSampleIndex = 0;
                }
            }
        }

        int samplesRemainingInBlock = static_cast<int>(currentBlock.size()) - currentSampleIndex;
        int samplesToCopy = std::min(numSamples - samplesWritten, samplesRemainingInBlock);
        for (int channel = 0; channel < numChannels; ++channel) {
            auto writePointer = buffer.getWritePointer(channel, samplesWritten);
            for (int sample = 0; sample < samplesToCopy; ++sample) {
                writePointer[sample] = currentBlock[currentSampleIndex + sample];
            }
        }

        samplesWritten += samplesToCopy; {
            juce::ScopedLock sl(lock);
            currentSampleIndex += samplesToCopy;

            if (currentSampleIndex >= static_cast<int>(currentBlock.size())) {
                currentBlock.clear();
                currentSampleIndex = 0;
            }
        }
    }
}

void MainAudioProcessor::onAudioBlockReceivedDecoded(const AudioBlockReceivedDecodedEvent &event) {
    juce::ScopedLock sl(lock);
    audioQueue.push_back(event.data);
}


#else
void MainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0) {
        return; // Rien à traiter
    }

    // Optimisation mémoire : éviter les réallocations fréquentes
    std::vector<float> pcmData;
    pcmData.reserve(static_cast<size_t>(numChannels * numSamples));

    // Copier les échantillons dans un format interleaved (L, R, L, R...)
    for (int sample = 0; sample < numSamples; ++sample) {
        for (int channel = 0; channel < numChannels; ++channel) {
            pcmData.push_back(buffer.getReadPointer(channel)[sample]);
        }
    }

    if (pcmData.empty()) return; // Vérification plus explicite

    // Envoyer l'événement avec std::move pour éviter les copies inutiles
    EventManager::getInstance().notifyAudioBlockProcessed(AudioBlockProcessedEvent{
        std::move(pcmData), // Transfert des données
        numChannels,
        numSamples,
        getSampleRate()
    });
}
#endif


//==============================================================================
bool MainAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *MainAudioProcessor::createEditor() {
    return new MainApplication(*this);
}

//==============================================================================
void MainAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void MainAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor * JUCE_CALLTYPE createPluginFilter() {
    return new MainAudioProcessor();
}
