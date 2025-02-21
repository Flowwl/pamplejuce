#pragma once
#include <vector>
#include <stdexcept>

template <typename T>
class CircularBuffer
{
public:
    // Constructeur : la capacité est fixée lors de l'instanciation
    explicit CircularBuffer(int capacity)
        : buffer(capacity), capacity(capacity), readIndex(0), writeIndex(0), availableSamples(0)
    {
        if (capacity <= 0)
            throw std::invalid_argument("La capacité doit être supérieure à 0");
    }

    // Ajoute des échantillons dans le tampon
    // Renvoie le nombre d'échantillons effectivement ajoutés (si le tampon est plein, certains échantillons ne seront pas ajoutés)
    int pushSamples(const T* data, const int numSamples)
    {
        int samplesPushed = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            if (availableSamples < capacity)
            {
                buffer[writeIndex] = data[i];
                writeIndex = (writeIndex + 1) % capacity;
                ++availableSamples;
                ++samplesPushed;
            }
            else
            {
                // Le tampon est plein, on arrête l'insertion
                break;
            }
        }
        return samplesPushed;
    }

    // Retire des échantillons du tampon et les copie dans destination.
    // Renvoie le nombre d'échantillons effectivement lus.
    int popSamples(T* destination, const int numSamples)
    {
        int samplesPopped = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            if (availableSamples > 0)
            {
                destination[i] = buffer[readIndex];
                readIndex = (readIndex + 1) % capacity;
                --availableSamples;
                ++samplesPopped;
            }
            else
            {
                // Plus d'échantillons disponibles
                break;
            }
        }
        return samplesPopped;
    }

    // Renvoie le nombre d'échantillons actuellement disponibles dans le tampon
    int getNumAvailableSamples() const { return availableSamples; }

    // Renvoie l'espace libre (nombre d'échantillons pouvant être ajoutés)
    int getFreeSpace() const { return capacity - availableSamples; }

    // Vide le tampon
    void clear()
    {
        readIndex = 0;
        writeIndex = 0;
        availableSamples = 0;
    }

private:
    std::vector<T> buffer;
    int capacity;
    int readIndex;
    int writeIndex;
    int availableSamples;
};