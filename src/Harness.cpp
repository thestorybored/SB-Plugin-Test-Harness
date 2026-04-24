#include "Harness.h"

#include "Logging.h"
#include "Scenario.h"

#include <chrono>
#include <thread>

namespace sbharness {

Harness::Harness(HarnessConfig c)
    : cfg(std::move(c))
    , junit("plugin-harness", cfg.junitOut)
    , rngState(cfg.seed)
{
    Logger::setVerbose(cfg.verbose);
    if (cfg.pluginPath.isNotEmpty())
        bridge.openFromPluginPath(cfg.pluginPath);
}

std::unique_ptr<juce::AudioPluginInstance> Harness::createInstance()
{
    juce::String err;
    auto inst = loader.load(cfg.pluginPath, cfg.format,
                            cfg.sampleRate, cfg.blockSize, err);
    if (inst == nullptr) {
        Logger::error("createInstance failed: " + err);
        return nullptr;
    }
    return inst;
}

void Harness::prepare(juce::AudioPluginInstance& inst)
{
    inst.enableAllBuses();
    inst.setRateAndBufferSizeDetails(cfg.sampleRate, cfg.blockSize);
    inst.prepareToPlay(cfg.sampleRate, cfg.blockSize);
}

int Harness::blocksForMilliseconds(int ms) const
{
    const double samples = (cfg.sampleRate * static_cast<double>(ms)) / 1000.0;
    return juce::jmax(1, static_cast<int>(std::ceil(samples / cfg.blockSize)));
}

void Harness::processBlocks(juce::AudioPluginInstance& inst, int blockCount)
{
    const int numIn   = juce::jmax(1, inst.getTotalNumInputChannels());
    const int numOut  = juce::jmax(1, inst.getTotalNumOutputChannels());
    const int numCh   = juce::jmax(numIn, numOut);

    juce::AudioBuffer<float> buffer(numCh, cfg.blockSize);
    juce::MidiBuffer midi;

    for (int i = 0; i < blockCount; ++i) {
        buffer.clear();
        midi.clear();
        inst.processBlock(buffer, midi);
    }
}

void Harness::processFor(juce::AudioPluginInstance& inst, int ms)
{
    processBlocks(inst, blocksForMilliseconds(ms));
}

int Harness::run(Scenario& scenario)
{
    Logger::info("Running scenario: " + scenario.name()
                 + " (seed=" + juce::String(cfg.seed) + ")");

    if (scenario.requiresTestHooks() && ! bridge.available()) {
        Logger::warn("Scenario '" + scenario.name()
                     + "' requires STORYBORED_TEST_HOOKS=ON build; skipping.");
        TestCase tc;
        tc.classname = "harness";
        tc.name      = scenario.name() + ".skipped";
        tc.failureMessage = ""; // skipped passes
        junit.addCase(std::move(tc));
        junit.write();
        return 0;
    }

    const auto start = std::chrono::steady_clock::now();
    int failures = 0;
    juce::String err;
    try {
        failures = scenario.run(*this);
    } catch (const std::exception& e) {
        err = juce::String("exception: ") + e.what();
        Logger::error("Scenario threw: " + err);
        failures = 1;
    } catch (...) {
        err = "unknown exception";
        Logger::error("Scenario threw unknown exception");
        failures = 1;
    }
    const auto end = std::chrono::steady_clock::now();

    TestCase tc;
    tc.classname    = "harness";
    tc.name         = scenario.name();
    tc.durationSecs = std::chrono::duration<double>(end - start).count();
    if (failures > 0)
        tc.failureMessage = (err.isNotEmpty() ? err
                             : "scenario reported " + juce::String(failures) + " failure(s)");
    junit.addCase(std::move(tc));
    junit.write();

    Logger::info("Scenario '" + scenario.name() + "' "
                 + (failures == 0 ? "PASSED" : "FAILED ("
                                              + juce::String(failures) + " failures)"));
    return failures == 0 ? 0 : 1;
}

} // namespace sbharness
