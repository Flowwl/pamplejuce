#pragma once

class AudioSettings
{
public:
    static AudioSettings& getInstance() {
        static AudioSettings instance;
        return instance;
    }

    [[nodiscard]] int getSampleRate() const noexcept {
        return sampleRate;
    }

    [[nodiscard]] int getNumChannels() const noexcept {
        return numChannels;
    }

    [[nodiscard]] int getBlockSize() const noexcept {
        return blockSize;
    }

    [[nodiscard]] int getBitDepth() const noexcept {
        return bitDepth;
    }

    [[nodiscard]] int getOpusSampleRate() const noexcept {
        return opusSampleRate;
    }

    // Mutateurs pour définir les paramètres audio
    void setSampleRate(int newSampleRate) noexcept {
        sampleRate = newSampleRate;
    }

    void setOpusSampleRate(int newOpusSampleRate) noexcept {
        opusSampleRate = newOpusSampleRate;
    }

    void setBlockSize(int newBlockSize) noexcept {
        blockSize = newBlockSize;
    }

    void setNumChannels(int newNumChannels) noexcept {
        numChannels = newNumChannels;
    }

    void setBitDepth(int newBitDepth) noexcept {
        bitDepth = newBitDepth;
    }

    void setLatency(int newLatency) noexcept {
        latency = newLatency;
    }

    [[nodiscard]] int getLatency() const noexcept {
        return latency;
    }

    void setOpusBitRate(int newOpusBitRate) noexcept {
        opusBitRate = newOpusBitRate;
    }

    [[nodiscard]] int getOpusBitRate() const noexcept {
        return opusBitRate;
    }

private:
    // Constructeur et destructeur privés pour le Singleton
    AudioSettings() = default;
    ~AudioSettings() = default;

    // Suppression des méthodes de copie et d'affectation pour garantir l'unicité
    AudioSettings(const AudioSettings&) = delete;
    AudioSettings& operator=(const AudioSettings&) = delete;

    // Données membres pour stocker les paramètres audio
    int sampleRate = 44100;
    int blockSize = 256;
    int numChannels = 2;
    int bitDepth = 16;
    int opusSampleRate = 48000;
    int latency = 20;
    int opusBitRate = 64000;
};
