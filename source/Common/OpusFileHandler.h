#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <opus.h>
#include <juce_core/juce_core.h>

class OpusFileHandler {
public:
    OpusFileHandler(const uint32_t sampleRate, const int bitrate, const int16_t numChannels): sampleRate(sampleRate), bitrate(bitrate), numChannels(numChannels), file(nullptr), encoder(nullptr) {}
    static std::string getFilePath(const std::string& filename) {
        const char* homeDir = getenv("HOME");
        if (!homeDir) {
            throw std::runtime_error("Impossible de localiser le répertoire personnel (HOME).");
        }

        return std::string(homeDir) + "/Desktop/" + filename;
    }

    bool create(const std::string& filename) {
        const std::string filepath = getFilePath(filename);
        std::remove(filepath.c_str());
        juce::Logger::outputDebugString("Creating opus file: " + filename);
        file = new std::ofstream(filepath, std::ios::binary);
        if (!file->is_open()) {
            std::cerr << "Erreur : Impossible d'ouvrir le fichier." << std::endl;
            return false;
        }

        int error;
        encoder = opus_encoder_create(sampleRate, numChannels, OPUS_APPLICATION_AUDIO, &error);
        if (error != OPUS_OK) {
            std::cerr << "Erreur : Impossible de créer l'encodeur Opus." << std::endl;
            delete file;
            file = nullptr;
            return false;
        }

        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
        return true;
    }

    void write(const std::vector<unsigned char>& samples) const {
        if (!file || !file->is_open() || !encoder) return;
        if (samples.empty()) return;
        juce::Logger::outputDebugString("Writing in " + juce::String(samples.size()) + " bytes in opus file");
        file->write(reinterpret_cast<const std::ostream::char_type *>(samples.data()), samples.size());
        if (file->is_open()) {
            file->flush();
        }
    }

    void close() {
        if (file && file->is_open()) {
            juce::Logger::outputDebugString("Closing opus file");
            file->close();
            delete file;
            file = nullptr;
        }
        if (encoder) {
            opus_encoder_destroy(encoder);
            encoder = nullptr;
        }
    }

private:
    uint32_t sampleRate;
    int bitrate;
    uint16_t numChannels;
    std::ofstream* file;
    OpusEncoder* encoder;
};
