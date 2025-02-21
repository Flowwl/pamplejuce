#pragma once
#include <vector>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h> // Pour htons et htonl

// Constants
constexpr int RTP_VERSION = 2;
constexpr int PAYLOAD_TYPE = 111;
constexpr int PADDING = 0;
constexpr int EXTENSION = 0;
constexpr int CC = 0;
constexpr int MARKER = 0;

struct RTPHeader {
    uint8_t v_p_x_cc; // Version (2 bits), Padding (1 bit), Extension (1 bit), CSRC Count (4 bits)
    uint8_t m_pt; // Marker (1 bit), Payload Type (7 bits)
    uint16_t sequenceNumber; // Sequence Number
    uint32_t timestamp; // Timestamp
    uint32_t ssrc; // Synchronization Source Identifier
};

class RTPWrapper {
public:
    static std::vector<uint8_t> createRTPPacket(const std::vector<unsigned char> &audioData, uint16_t seqNum,
                                                uint32_t ts, uint32_t ssrc) {
        RTPHeader header;
        header.v_p_x_cc = (RTP_VERSION << 6) | (PADDING << 5) | (EXTENSION << 4) | CC;
        // Version=2, Padding=0, Extension=0, CC=0
        header.m_pt = (MARKER << 7) | PAYLOAD_TYPE; // Marker=0, PayloadType=111
        header.sequenceNumber = htons(seqNum);
        header.timestamp = htonl(ts);
        header.ssrc = htonl(ssrc);

        // Construire le paquet RTP
        std::vector<uint8_t> packet(sizeof(RTPHeader) + audioData.size());
        memcpy(packet.data(), &header, sizeof(RTPHeader)); // Copier l'en-tête RTP
        memcpy(packet.data() + sizeof(RTPHeader), audioData.data(), audioData.size()); // Ajouter les données audio

        return packet;
    }

    static constexpr uint8_t RTP_VERSION = 2;
    static constexpr size_t RTP_MIN_HEADER_SIZE = 12;

    static std::vector<unsigned char> removeRTPHeader(const std::vector<unsigned char> &rtpPacket) {
        // Vérifier que le paquet est assez grand pour contenir le header minimal
        if (rtpPacket.size() < RTP_MIN_HEADER_SIZE) {
            throw std::runtime_error("Invalid RTP packet: too small to contain a header");
        }

        // Le premier octet contient : version (bits 7-6), padding (bit 5), extension (bit 4) et CSRC count (bits 3-0)
        const unsigned char v_p_x_cc = rtpPacket[0];

        // Extraire la version RTP
        const unsigned char version = (v_p_x_cc >> 6) & 0x03;
        if (version != RTP_VERSION) {
            throw std::runtime_error("Invalid RTP packet: unsupported version");
        }

        // Extraire le CSRC count (nombre d'identifiants CSRC)
        const unsigned char csrcCount = v_p_x_cc & 0x0F;

        // Taille de l'en-tête RTP de base : 12 octets + 4 octets par CSRC
        size_t headerSize = RTP_MIN_HEADER_SIZE + (csrcCount * 4);

        // Vérifier le flag d'extension (bit 4 du premier octet)
        const unsigned char extensionFlag = (v_p_x_cc >> 4) & 0x01;
        if (extensionFlag) {
            // Pour une extension, il faut au moins 4 octets supplémentaires pour l'en-tête d'extension
            if (rtpPacket.size() < headerSize + 4) {
                throw std::runtime_error("Invalid RTP packet: not enough data for extension header");
            }
            // L'en-tête d'extension se compose de 4 octets : 16 bits de profil et 16 bits de longueur (en mots 32 bits)
            // La longueur est située aux octets headerSize+2 et headerSize+3 (en big-endian)
            uint16_t extensionLength = (static_cast<uint16_t>(rtpPacket[headerSize + 2]) << 8) |
                                       static_cast<uint16_t>(rtpPacket[headerSize + 3]);
            // La taille de l'extension est de 4 octets pour le header + extensionLength * 4 octets supplémentaires
            headerSize += 4 + (extensionLength * 4);
        }

        // Vérifier que le paquet contient des données payload après l'en-tête
        if (rtpPacket.size() <= headerSize) {
            throw std::runtime_error("Invalid RTP packet: no payload data");
        }

        // Retourner uniquement le payload (sans l'en-tête RTP)
        return std::vector<unsigned char>(rtpPacket.begin() + headerSize, rtpPacket.end());
    }
};
