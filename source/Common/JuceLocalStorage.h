#pragma once

#include <juce_core/juce_core.h>
#include <memory>
#include "../Config.h"

class JuceLocalStorage
{
public:
    static JuceLocalStorage& getInstance()
    {
#ifdef MELO_PLUGIN_NAME
        static JuceLocalStorage instance(MELO_PLUGIN_NAME);
#else
        static JuceLocalStorage instance("MeloVST");
#endif
        return instance;
    }

    void saveValue(const juce::String& key, const juce::String& value) const {
        propertiesFile->setValue(key, value);
        propertiesFile->saveIfNeeded();
        juce::Logger::outputDebugString("Value saved for key: " + key);
    }

    juce::String loadValue(const juce::String& key) const
    {
        juce::String value = propertiesFile->getValue(key, {});
        if (value.isNotEmpty())
        {
            juce::Logger::outputDebugString("Value loaded for key: " + key + " - " + value);
        }
        else
        {
            juce::Logger::outputDebugString("No value found for key: " + key);
        }
        return value;
    }

    void removeValue(const juce::String& key) const {
        propertiesFile->removeValue(key);
        propertiesFile->saveIfNeeded();
        juce::Logger::outputDebugString("Value removed for key: " + key);
    }

private:
    explicit JuceLocalStorage(const juce::String& appName)
    {
        // Configure les options de fichier de propriétés
        juce::PropertiesFile::Options options;
        options.applicationName = appName;
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";

        propertiesFile = std::make_unique<juce::PropertiesFile>(options);
    }

    // Supprime le constructeur de copie et le constructeur d'affectation
    JuceLocalStorage(const JuceLocalStorage&) = delete;
    JuceLocalStorage& operator=(const JuceLocalStorage&) = delete;

    static JuceLocalStorage* instance;
    std::unique_ptr<juce::PropertiesFile> propertiesFile;
};
