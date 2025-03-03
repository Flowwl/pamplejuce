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

        double resampleRatio = static_cast<double>(targetSampleRate) / sourceSampleRate;
        size_t outSize = static_cast<size_t>(std::ceil(sourceVector.size() * resampleRatio));

        std::vector<double> sourceDouble(sourceVector.begin(), sourceVector.end());
        std::vector<double> outDouble(outSize, 0.0);
        auto outData = outDouble.data();

        int processedSamples = resampler->process(sourceDouble.data(), sourceDouble.size(), outData);
        if (processedSamples < 0) {
            throw std::runtime_error("Resampler returned an error.");
        }

        std::vector<float> outVector(outDouble.begin(), outDouble.begin() + processedSamples);
        return outVector;
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
