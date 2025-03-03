#pragma once
#include <opus.h>
#include <vector>
#include <stdexcept>
#include <functional>

#define MAX_OPUS_PACKET_SIZE 1500

class OpusEncoderWrapper {
public:
    OpusEncoderWrapper(const int sample_rate, const int channels, const int bitrate): numChannels(channels), sampleRate(sample_rate) {
        int error;
        encoder = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &error);
        if (error != OPUS_OK)
            throw std::runtime_error("Failed to create Opus encoder: " + std::string(opus_strerror(error)));

        // Configuration de l'encodeur
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
        opus_encoder_ctl(encoder, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl (encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
        opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(10));
        opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(5));
        opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(1));
        opus_encoder_ctl(encoder, OPUS_SET_DTX(0));

    }

    ~OpusEncoderWrapper() {
        if (encoder)
            opus_encoder_destroy(encoder);
    }

    std::vector<unsigned char> encode_float(const std::vector<float>& pcm, const int frameSize) {
        std::vector<unsigned char> res(4000, 0);
        //check if the number of samples is valid use the available frame sizes
        int nextFrameSize = findNextAvailableFrameSize(frameSize);
        if (std::find(availableframeSizes.begin(), availableframeSizes.end(), nextFrameSize) == availableframeSizes.end()) {
            DBG ("Invalid frame size: " + std::to_string(frameSize));
            return {};
        }
        const int ret = opus_encode_float(encoder, pcm.data(), nextFrameSize, res.data(), static_cast<int>(res.size()));
        if (ret < 0) {
            return {};
        }

        res.resize(ret);
        return res;
    }

    int findNextAvailableFrameSize(const int frameSize) {
        const auto it = std::upper_bound(availableframeSizes.begin(), availableframeSizes.end(), frameSize);

        if (it != availableframeSizes.end()) {
            return *it;  // Retourne le prochain frameSize disponible
        }
        return availableframeSizes.back();  // Si frameSize est trop grand, retourne le dernier
    }

private:
    std::vector<int> availableframeSizes = {120, 240, 480, 960, 1920, 2880};
    std::vector<int16_t> in_buffer_;
    std::vector<float> in_buffer_float_;
    OpusEncoder *encoder = nullptr;
    int numChannels;
    int sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpusEncoderWrapper)

};
