#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

namespace sbharness {

class PluginLoader {
public:
    PluginLoader();

    // Returns nullptr on failure with `errorOut` populated.
    std::unique_ptr<juce::AudioPluginInstance>
        load(const juce::String& path,
             const juce::String& formatName,
             double sampleRate,
             int blockSize,
             juce::String& errorOut);

    // Available format names (lowercase): "vst3", "au" (macOS), ...
    juce::StringArray availableFormats() const;

private:
    juce::AudioPluginFormatManager formatManager;
};

} // namespace sbharness
