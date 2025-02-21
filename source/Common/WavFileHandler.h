#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>    // Pour std::memcpy
#include <stdexcept>
#include <cstdio>     // Pour std::remove
#include <cstdlib>    // Pour getenv
#include <juce_core/juce_core.h>

class WavFileHandler {
public:
    WavFileHandler(const int sampleRate, const int numChannels)
        : file(nullptr),
          sampleRate(sampleRate),
          bitsPerSample(32),
          numChannels(numChannels),
          dataSize(0) {}

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
        juce::Logger::outputDebugString("Creating wav file: " + filename);
        file = new std::ofstream(filepath, std::ios::binary);
        if (!file->is_open()) {
            std::cerr << "Erreur : Impossible d'ouvrir le fichier." << std::endl;
            return false;
        }
        writeWavHeader();
        return true;
    }

    // Écriture des échantillons en float directement dans le fichier
    void write(const std::vector<float>& samples, const bool isMono) {
        if (file && file->is_open()) {
            std::vector<float> processedSamples;

            if (isMono) {
                // Convertir mono → stéréo (dupliquer chaque échantillon)
                processedSamples.reserve(samples.size() * 2);
                for (float sample : samples) {
                    processedSamples.push_back(sample); // Gauche
                    processedSamples.push_back(sample); // Droite
                }
            } else {
                // Vérifier que l'entrée stéréo a un nombre d'échantillons pair
                if (samples.size() % 2 != 0) {
                    std::cerr << "Erreur : Nombre d'échantillons impair pour un format stéréo !" << std::endl;
                    return;
                }
                processedSamples = samples; // Utiliser directement les données
            }

            // Écriture dans le fichier
            file->write(reinterpret_cast<const char*>(processedSamples.data()), processedSamples.size() * sizeof(float));
            dataSize += processedSamples.size() * sizeof(float);
            file->flush();
        }
    }

    void close() {
        if (file && file->is_open()) {
            juce::Logger::outputDebugString("Closing Wav file");
            finalizeWavHeader();
            file->close();
            delete file;
            file = nullptr;
        }
    }

private:
    std::ofstream* file;
    uint32_t sampleRate;
    uint16_t bitsPerSample;
    uint16_t numChannels;
    uint32_t dataSize;

    // Écrit un en-tête WAV initial avec des tailles par défaut (0) à mettre à jour à la fermeture
    void writeWavHeader() const {
        char header[44] = {0};
        // RIFF Chunk Descriptor
        std::memcpy(header, "RIFF", 4);
        constexpr uint32_t chunkSize = 36; // 36 + dataSize, dataSize est initialement 0
        std::memcpy(header + 4, &chunkSize, 4);
        std::memcpy(header + 8, "WAVE", 4);
        // fmt subchunk
        std::memcpy(header + 12, "fmt ", 4);
        constexpr uint32_t subChunk1Size = 16;
        std::memcpy(header + 16, &subChunk1Size, 4);
        // Pour des données float, audioFormat doit être 3 (WAVE_FORMAT_IEEE_FLOAT)
        constexpr uint16_t audioFormat = 3;
        std::memcpy(header + 20, &audioFormat, 2);
        std::memcpy(header + 22, &numChannels, 2);
        std::memcpy(header + 24, &sampleRate, 4);
        const uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
        std::memcpy(header + 28, &byteRate, 4);
        const uint16_t blockAlign = numChannels * bitsPerSample / 8;
        std::memcpy(header + 32, &blockAlign, 2);
        std::memcpy(header + 34, &bitsPerSample, 2);
        // data subchunk
        std::memcpy(header + 36, "data", 4);
        constexpr uint32_t subChunk2Size = 0;
        std::memcpy(header + 40, &subChunk2Size, 4);

        file->write(header, 44);
    }

    // Met à jour l'en-tête WAV avec la taille réelle des données
    void finalizeWavHeader() const {
        file->seekp(4, std::ios::beg);
        const uint32_t chunkSize = 36 + dataSize;
        file->write(reinterpret_cast<const char*>(&chunkSize), 4);
        file->seekp(40, std::ios::beg);
        file->write(reinterpret_cast<const char*>(&dataSize), 4);
    }
};
