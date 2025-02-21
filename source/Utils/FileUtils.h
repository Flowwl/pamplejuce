#pragma once
#include <juce_core/juce_core.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <opus_types.h>

namespace FileUtils {
    // Génère un nom de fichier avec un horodatage
    static std::string generateTimestampedFilename(const std::string& baseName, const std::string& extension) {
        // Obtenir l'heure actuelle
        const auto now = std::chrono::system_clock::now();
        const auto timeT = std::chrono::system_clock::to_time_t(now);
        const auto localTime = *std::localtime(&timeT);

        // Formater l'horodatage
        std::ostringstream oss;
        oss << baseName << "_"
            << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") // Format : Année-Mois-Jour_Heure-Minute-Seconde
            << "." << extension;

        return oss.str();
    }

    // Obtient le chemin du fichier dans le répertoire Desktop de l'utilisateur
    inline std::string getFilePath(const std::string& filename) {
        const char* homeDir = getenv("HOME");
        if (!homeDir) {
            throw std::runtime_error("Impossible de localiser le répertoire personnel (HOME).");
        }

        return std::string(homeDir) + "/Desktop/" + filename;
    }

    // Initialise un fichier Ogg Opus pour écrire les données
    inline std::fstream initializeOpusFile(const std::string& filename) {
        const std::string filepath = getFilePath(filename);
        std::remove(filepath.c_str()); // Supprime le fichier s'il existe déjà

        std::fstream opusFile(filepath, std::ios::binary | std::ios::out);
        if (!opusFile) {
            throw std::runtime_error("Failed to create Opus file.");
        }

        // Écrire l'en-tête Ogg
        opusFile.write("OggS", 4); // Identifiant Ogg
        opusFile.put(0); // Version
        opusFile.put(0); // Type de paquet (0 pour les données)
        opusFile.put(0); // Granule position (à définir plus tard)
        opusFile.put(0); // Granule position (à définir plus tard)

        // Numéro de série de bitstream (doit être un entier positif unique)
        opusFile.put(0); // Numéro de série (à définir plus tard)
        opusFile.put(0); // Numéro de série (à définir plus tard)

        // Écrire des informations de longueur de page (0 pour le moment)
        opusFile.put(0); // Index de page (à définir plus tard)
        opusFile.put(0); // Index de page (à définir plus tard)

        return opusFile;
    }

    // Ajoute des données Opus au fichier
    inline void appendOpusData(std::fstream& opusFile, const std::vector<unsigned char>& opusData) {
        if (!opusFile.is_open()) {
            throw std::runtime_error("Opus file is not open for writing.");
        }

        // Écrire les données Opus
        opusFile.write(reinterpret_cast<const char*>(opusData.data()), opusData.size());

        // Mettre à jour les informations dans l'en-tête Ogg
        const std::streamsize opusDataSize = opusData.size();
        opusFile.seekp(0, std::ios::beg); // Retourner au début pour modifier l'en-tête

        // Écrire à nouveau l'en-tête Ogg
        opusFile.write("OggS", 4);
        opusFile.put(0); // Version
        opusFile.put(0); // Type de paquet
        opusFile.put(static_cast<char>((opusDataSize >> 24) & 0xFF)); // Granule position
        opusFile.put(static_cast<char>((opusDataSize >> 16) & 0xFF)); // Granule position
        opusFile.put(static_cast<char>((opusDataSize >> 8) & 0xFF));  // Granule position
        opusFile.put(static_cast<char>(opusDataSize & 0xFF)); // Granule position
        opusFile.put(0); // Bitstream serial number (à définir)
        opusFile.put(0); // Index de page
        opusFile.put(0); // Index de page
    }

    // Finalise le fichier Ogg Opus
    inline void finalizeOpusFile(std::fstream& opusFile, const std::streamsize opusDataSize, const opus_int32 bitstreamSerialNumber = 12345) {
        if (!opusFile.is_open()) {
            return;
        }

        // Retourner au début pour mettre à jour l'en-tête
        opusFile.seekp(0, std::ios::beg);

        // Mettre à jour l'en-tête Ogg
        opusFile.write("OggS", 4); // Identifiant Ogg
        opusFile.put(0); // Version
        opusFile.put(0); // Type de paquet
        opusFile.put(static_cast<char>((opusDataSize >> 24) & 0xFF)); // Granule position (taille des données)
        opusFile.put(static_cast<char>((opusDataSize >> 16) & 0xFF)); // Granule position
        opusFile.put(static_cast<char>((opusDataSize >> 8) & 0xFF));  // Granule position
        opusFile.put(static_cast<char>(opusDataSize & 0xFF));          // Granule position
        opusFile.put(static_cast<char>((bitstreamSerialNumber >> 24) & 0xFF)); // Numéro de série de bitstream
        opusFile.put(static_cast<char>((bitstreamSerialNumber >> 16) & 0xFF)); // Numéro de série de bitstream
        opusFile.put(static_cast<char>((bitstreamSerialNumber >> 8) & 0xFF));  // Numéro de série de bitstream
        opusFile.put(static_cast<char>(bitstreamSerialNumber & 0xFF)); // Numéro de série de bitstream

        opusFile.close(); // Fermer le fichier
    }

    // Initialise un fichier WAV pour écrire les données
    inline std::fstream initializeWavFile(const std::string& filename) {
        const std::string filepath = getFilePath(filename);
        std::remove(filepath.c_str()); // Supprime le fichier s'il existe déjà

        std::fstream wavFile(filepath, std::ios::binary | std::ios::out);
        wavFile.open(filepath, std::ios::binary | std::ios::out);
        if (!wavFile.is_open()) {
            throw std::runtime_error("Failed to create WAV file.");
        }

        constexpr int sampleRate = 44100; // Exemple : fréquence d'échantillonnage
        constexpr int channels = 2; // Exemple : nombre de canaux
        juce::Logger::outputDebugString(
            "Initializing WAV file with " + std::to_string(sampleRate) + "Hz, " + std::to_string(channels) + " channels");

        // Paramètres pour l'entête initial
        constexpr int byteRate = sampleRate * channels * sizeof(int16_t);
        constexpr int blockAlign = channels * sizeof(int16_t);
        constexpr int dataSize = 0; // Initialement vide
        constexpr int chunkSize = 36 + dataSize;

        // Écrire un en-tête provisoire
        wavFile.write("RIFF", 4);
        wavFile.write(reinterpret_cast<const char*>(&chunkSize), 4);
        wavFile.write("WAVE", 4);
        wavFile.write("fmt ", 4);

        constexpr int subchunk1Size = 16; // PCM
        constexpr int audioFormat = 1; // Linear PCM
        wavFile.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
        wavFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
        wavFile.write(reinterpret_cast<const char*>(&channels), 2);
        wavFile.write(reinterpret_cast<const char*>(&sampleRate), 4);
        wavFile.write(reinterpret_cast<const char*>(&byteRate), 4);
        wavFile.write(reinterpret_cast<const char*>(&blockAlign), 2);

        constexpr int bitsPerSample = 16;
        wavFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
        wavFile.write("data", 4);
        wavFile.write(reinterpret_cast<const char*>(&dataSize), 4);
        return wavFile;
    }

    // Ajoute des données WAV au fichier
    inline void appendWavData(std::fstream& wavFile, const std::vector<int16_t>& pcmData) {
        if (!wavFile.is_open()) {
            return;
        }

        wavFile.write(reinterpret_cast<const char*>(pcmData.data()), pcmData.size() * sizeof(int16_t));
    }

    // Finalise le fichier WAV
    inline void finalizeWavFile(std::fstream& wavFile) {
        // Aller à la fin pour calculer la taille des données
        wavFile.seekp(0, std::ios::end);
        const int fileSize = wavFile.tellp();
        const int dataSize = fileSize - 44; // Taille des données après l'en-tête

        // Mettre à jour la taille des données et le chunkSize
        wavFile.seekp(4, std::ios::beg);
        const int chunkSize = 36 + dataSize;
        wavFile.write(reinterpret_cast<const char*>(&chunkSize), 4);
        wavFile.seekp(40, std::ios::beg);
        wavFile.write(reinterpret_cast<const char*>(&dataSize), 4);

        wavFile.close(); // Fermer le fichier
    }
}
