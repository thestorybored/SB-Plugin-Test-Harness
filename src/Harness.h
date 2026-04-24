#pragma once

#include "JUnitReport.h"
#include "PluginLoader.h"
#include "TestHookBridge.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <chrono>
#include <memory>
#include <random>

namespace sbharness {

struct HarnessConfig {
    juce::String pluginPath;
    juce::String format       = "VST3";
    juce::String hostName     = "StoryBored Harness";
    juce::String junitOut;
    juce::String scenario;
    int          instances    = 4;
    int          durationSecs = 5;
    int          iterations   = 10;
    int          blockSize    = 512;
    double       sampleRate   = 48000.0;
    uint64_t     seed         = 0;
    bool         seedExplicit = false;
    bool         verbose      = false;
    bool         withEditor   = false;
};

class Scenario;

class Harness {
public:
    explicit Harness(HarnessConfig cfg);

    int run(Scenario& scenario);

    // Scenario API ----------------------------------------------------------
    std::unique_ptr<juce::AudioPluginInstance> createInstance();
    void prepare(juce::AudioPluginInstance& inst);
    void processBlocks(juce::AudioPluginInstance& inst, int blockCount);
    void processFor(juce::AudioPluginInstance& inst, int milliseconds);

    int blocksForMilliseconds(int ms) const;

    const HarnessConfig& config() const { return cfg; }
    TestHookBridge&      hooks()        { return bridge; }
    JUnitReport&         report()       { return junit; }
    std::mt19937_64&     rng()          { return rngState; }

private:
    HarnessConfig    cfg;
    PluginLoader     loader;
    TestHookBridge   bridge;
    JUnitReport      junit;
    std::mt19937_64  rngState;
};

} // namespace sbharness
