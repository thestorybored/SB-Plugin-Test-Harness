#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace sbharness {

// ABI v1 - frozen. New hooks added, never changed.
// Plugins built with STORYBORED_TEST_HOOKS=ON export:
//
//     extern "C" StoryBoredTestHooksV1*
//     storybored_test_hooks_v1(juce::AudioPluginInstance*);
//
// Returns nullptr if hooks aren't available for that instance.
struct StoryBoredTestHooksV1 {
    int   abiVersion;                                                         // = 1
    bool  (*activateLicenseAsync)(juce::AudioPluginInstance*, const char*);
    bool  (*deactivateLicenseAsync)(juce::AudioPluginInstance*);
    bool  (*triggerAnalyticsFlush)(juce::AudioPluginInstance*);
    bool  (*triggerSentryTestEvent)(juce::AudioPluginInstance*, const char*);
    int   (*getActiveSentryInstanceCount)();
    int   (*getActivePostHogInstanceCount)();
    int   (*getActiveHTTPRequestCount)();
};

class TestHookBridge {
public:
    TestHookBridge();
    ~TestHookBridge();
    TestHookBridge(const TestHookBridge&)            = delete;
    TestHookBridge& operator=(const TestHookBridge&) = delete;

    // Resolves the symbol on the plugin's binary. Returns true if found.
    // `pluginPath` should be the same .vst3/.component/.clap path passed to PluginLoader.
    bool openFromPluginPath(const juce::String& pluginPath);

    bool available() const { return factory != nullptr; }

    // nullptr if !available() or the plugin doesn't expose hooks for this instance.
    StoryBoredTestHooksV1* get(juce::AudioPluginInstance& inst);

private:
    using Factory = StoryBoredTestHooksV1* (*)(juce::AudioPluginInstance*);
    void*   dsoHandle = nullptr;
    Factory factory   = nullptr;
};

} // namespace sbharness
