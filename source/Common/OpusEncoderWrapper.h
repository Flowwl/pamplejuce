#pragma once
#include <opus.h>
#include <vector>
#include <stdexcept>
#include <functional>

#define MAX_OPUS_PACKET_SIZE 1500

class OpusEncoderWrapper {
public:
    OpusEncoderWrapper(const int sample_rate, const int channels, const int duration_ms, const int bitrate): frameDurationInMs(duration_ms), numChannels(channels), sampleRate(sample_rate) {
        int error;
        encoder = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &error);
        if (error != OPUS_OK)
            throw std::runtime_error("Failed to create Opus encoder: " + std::string(opus_strerror(error)));

        // Configuration de l'encodeur
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
        // opus_encoder_ctl(encoder, OPUS_SET_BITRATE(OPUS_AUTO));
        // opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(0));
        // opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(5));
        // opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(0));
        // opus_encoder_ctl(encoder, OPUS_SET_DTX(0));
        // opus_decoder_ctl(decoder, OPUS_SET_GAIN(1));

        frameSizePerChannel = sampleRate / 1000 * frameDurationInMs;
        frame_size_ = sample_rate / 1000 * channels * duration_ms;
    }

    ~OpusEncoderWrapper() {
        if (encoder)
            opus_encoder_destroy(encoder);
    }

    std::vector<unsigned char> encode_float(const std::vector<float>& pcm, const int nbSamples) const {
        // On définit une taille maximale pour le paquet de sortie (par exemple 4000 octets)
        std::vector<unsigned char> res(4000, 0);

        const int ret = opus_encode_float(encoder, pcm.data(), nbSamples, res.data(), static_cast<int>(res.size()));
        if (ret < 0) {
            return res;
        }

        res.resize(ret);
        return res;
    }

private:
    std::vector<int16_t> in_buffer_;
    std::vector<float> in_buffer_float_;
    OpusEncoder *encoder = nullptr;
    int frame_size_;
    int frameSizePerChannel;
    int frameDurationInMs;
    int numChannels;
    int sampleRate;
};
