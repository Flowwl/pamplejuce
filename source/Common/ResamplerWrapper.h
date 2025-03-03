//
// Created by Padoa on 27/01/2025.
//

#pragma once
#include "../Config.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <r8brain/CDSPResampler.h>
#include <r8brain/r8bbase.h>
#include <vector>

class ResamplerWrapper {
public:
    ResamplerWrapper(double sourceSR, double targetSR, const int nChan) {
        resampler = std::make_unique<r8b::CDSPResampler>(sourceSR, targetSR, 512);
        sourceSampleRate = sourceSR;
        targetSampleRate = targetSR;
        numChannels = nChan;
    };

    std::vector<float> resampleFromFloat(const std::vector<float>& sourceVector) const {
        if (sourceSampleRate == targetSampleRate) {
            return sourceVector;
        }
        std::vector<double> doubleAudioBlock(sourceVector.size());
        std::transform(sourceVector.begin(), sourceVector.end(), doubleAudioBlock.begin(),
                       [](float sample) { return static_cast<double>(sample); });

        std::vector<double> resampledDoubleAudioBlock = resampleFromDouble(doubleAudioBlock);
        std::vector<float> resampledAudioBlock(resampledDoubleAudioBlock.size());
        std::transform(resampledDoubleAudioBlock.begin(), resampledDoubleAudioBlock.end(), resampledAudioBlock.begin(),
                       [](double sample) { return static_cast<float>(sample); });
        return resampledAudioBlock;
    }

    std::vector<double> resampleFromDouble(std::vector<double>& sourceVector) const {
        size_t outSize = std::ceil(targetSampleRate * Config::getInstance().latencyInMs / 1000 * numChannels);
        std::vector<double> outVector(outSize, 0.0);

        double* outBuffer = outVector.data();  // Utiliser le buffer du vecteur

        int processedSamples = resampler->process(sourceVector.data(), sourceVector.size(), outBuffer);

        if (processedSamples < 0) {
            throw std::runtime_error("Resampler returned an error.");
        }

        outVector.resize(processedSamples);
        return outVector;
    }

    double getScaleFactor() const {
        return targetSampleRate / sourceSampleRate;
    }

private:
    std::unique_ptr<r8b::CDSPResampler> resampler;
    double sourceSampleRate;
    double targetSampleRate;
    int numChannels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResamplerWrapper);
};
