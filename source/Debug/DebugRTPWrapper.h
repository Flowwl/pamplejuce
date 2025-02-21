#pragma once
#include <iostream>
#include "../Common/RTPWrapper.h"
#include <rtc/rtc.hpp>

struct DebugRTPHeader {
        uint8_t v_p_x_cc; // Version (2 bits), Padding (1 bit), Extension (1 bit), CSRC Count (4 bits)
        uint8_t m_pt;     // Marker (1 bit), Payload Type (7 bits)
        uint16_t sequenceNumber; // Sequence Number
        uint32_t timestamp; // Timestamp
        uint32_t ssrc;     // Synchronization Source Identifier
};

namespace DebugRTPWrapper {
    static void debugPacket(const std::vector<uint8_t> &encodedData) {
        typedef struct {
            unsigned char cc: 4; /* CSRC count             */
            unsigned char x: 1; /* header extension flag  */
            unsigned char p: 1; /* padding flag           */
            unsigned char version: 2; /* protocol version       */
            unsigned char pt: 7; /* payload type           */
            unsigned char m: 1; /* marker bit             */
            uint16_t seq; /* sequence number        */
            uint32_t ts; /* timestamp              */
            uint32_t ssrc; /* synchronization source */
        } srtp_hdr_t;

        try {
            const auto data = reinterpret_cast<const std::byte *>(encodedData.data());
            auto bin = rtc::binary(data, data + encodedData.size()); // message_variant
            const auto message = rtc::make_message(std::move(bin));

            const int size = static_cast<int>(message->size());
            const auto message2 = rtc::make_message(size + 16 + 128, message);
            void *message2Data = message2->data();
            const auto hdr = static_cast<srtp_hdr_t *>(message2Data);
            std::cout << "RTP Header:" << std::endl;
            std::cout << "  Version: " << static_cast<int>(hdr->version)
                    << " (0x" << std::hex << static_cast<int>(hdr->version) << std::dec << ")" << std::endl;

            std::cout << "  CC: " << static_cast<int>(hdr->cc)
                    << " (0x" << std::hex << static_cast<int>(hdr->cc) << std::dec << ")" << std::endl;

            std::cout << "  X: " << static_cast<int>(hdr->x)
                    << " (0x" << std::hex << static_cast<int>(hdr->x) << std::dec << ")" << std::endl;

            std::cout << "  Padding: " << static_cast<int>(hdr->p)
                    << " (0x" << std::hex << static_cast<int>(hdr->p) << std::dec << ")" << std::endl;

            std::cout << "  Payload Type: " << static_cast<int>(hdr->pt)
                    << " (0x" << std::hex << static_cast<int>(hdr->pt) << std::dec << ")" << std::endl;

            std::cout << "  Marker (M): " << static_cast<int>(hdr->m)
                    << " (0x" << std::hex << static_cast<int>(hdr->m) << std::dec << ")" << std::endl;

            std::cout << "  Sequence Number: " << hdr->seq
                    << " (0x" << std::hex << hdr->seq << std::dec << ")" << std::endl;

            std::cout << "  Timestamp: " << hdr->ts
                    << " (0x" << std::hex << hdr->ts << std::dec << ")" << std::endl;

            std::cout << "  SSRC: " << hdr->ssrc
                    << " (0x" << std::hex << hdr->ssrc << std::dec << ")" << std::endl;
        } catch (std::exception &e) {
            std::cout << ("Error decoding RTP header: ") << e.what() << std::endl;
        }
    }

    static void debugRTPHeader(const DebugRTPHeader &header) {
        std::cout << "RTP Header Debug:" << std::endl;
        std::cout << "  Version: " << ((header.v_p_x_cc >> 6) & 0x03) << std::endl;
        std::cout << "  Padding: " << ((header.v_p_x_cc >> 5) & 0x01) << std::endl;
        std::cout << "  Extension: " << ((header.v_p_x_cc >> 4) & 0x01) << std::endl;
        std::cout << "  CSRC Count: " << (header.v_p_x_cc & 0x0F) << std::endl;
        std::cout << "  Marker: " << ((header.m_pt >> 7) & 0x01) << std::endl;
        std::cout << "  Payload Type: " << (header.m_pt & 0x7F) << std::endl;
        std::cout << "  Sequence Number: " << ntohs(header.sequenceNumber) << std::endl;
        std::cout << "  Timestamp: " << ntohl(header.timestamp) << std::endl;
        std::cout << "  SSRC: " << ntohl(header.ssrc) << std::endl;
    }
}