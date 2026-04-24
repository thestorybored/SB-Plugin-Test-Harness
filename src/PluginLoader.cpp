#include "PluginLoader.h"

#include "Logging.h"

namespace sbharness {

PluginLoader::PluginLoader()
{
   #if JUCE_PLUGINHOST_VST3
    formatManager.addFormat(new juce::VST3PluginFormat());
   #endif
   #if JUCE_PLUGINHOST_AU && (JUCE_MAC || JUCE_IOS)
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
   #endif
}

juce::StringArray PluginLoader::availableFormats() const
{
    juce::StringArray names;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
        names.add(formatManager.getFormat(i)->getName());
    return names;
}

std::unique_ptr<juce::AudioPluginInstance>
PluginLoader::load(const juce::String& path,
                   const juce::String& formatName,
                   double sampleRate,
                   int blockSize,
                   juce::String& errorOut)
{
    juce::AudioPluginFormat* format = nullptr;
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        auto* f = formatManager.getFormat(i);
        if (f->getName().equalsIgnoreCase(formatName)) {
            format = f;
            break;
        }
    }
    if (format == nullptr) {
        errorOut = "Unknown format '" + formatName + "'. Available: "
                 + availableFormats().joinIntoString(", ");
        return nullptr;
    }

    juce::OwnedArray<juce::PluginDescription> descriptions;
    format->findAllTypesForFile(descriptions, path);
    if (descriptions.isEmpty()) {
        errorOut = "No " + formatName + " plugin descriptions found at " + path;
        return nullptr;
    }

    juce::String createError;
    auto instance = formatManager.createPluginInstance(
        *descriptions.getFirst(), sampleRate, blockSize, createError);
    if (instance == nullptr) {
        errorOut = "createPluginInstance failed: " + createError;
        return nullptr;
    }

    Logger::debug("Loaded plugin: " + descriptions.getFirst()->name
                  + " (" + format->getName() + ")");
    return instance;
}

} // namespace sbharness
