#include "CrashReporter.h"
#include "Harness.h"
#include "Logging.h"
#include "Scenario.h"

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <cstdio>
#include <iostream>
#include <random>

namespace sbharness {

namespace {

void printUsage()
{
    std::fputs(
        "plugin-harness " HARNESS_VERSION_STRING "\n"
        "Usage: plugin-harness [options]\n"
        "\n"
        "Required:\n"
        "  --plugin PATH                Path to the plugin (.vst3/.component/.clap).\n"
        "  --scenario NAME              Scenario to run. See --list-scenarios.\n"
        "\n"
        "Optional:\n"
        "  --format NAME                Plugin format (VST3, AudioUnit, CLAP). Default: VST3.\n"
        "  --instances N                Concurrent instances (default: 4).\n"
        "  --duration SECS              Wall-clock duration for time-based scenarios (default: 5).\n"
        "  --iterations N               Iteration count for repetition-based scenarios (default: 10).\n"
        "  --sample-rate RATE           Sample rate (default: 48000).\n"
        "  --block-size N               Block size in samples (default: 512).\n"
        "  --host-name NAME             Host name string (default: \"StoryBored Harness\").\n"
        "  --seed N                     RNG seed (default: random).\n"
        "  --junit-out PATH             Write JUnit XML to PATH.\n"
        "  --with-editor                Create the plugin editor as well.\n"
        "  --verbose                    Enable debug logging.\n"
        "  --list-scenarios             List available scenarios and exit.\n"
        "  --list-formats               List available plugin formats and exit.\n"
        "  --version                    Print version and exit.\n"
        "  --help                       Print this help and exit.\n",
        stdout);
}

void listScenarios()
{
    for (const auto& s : ScenarioFactory::all()) {
        std::printf("  %-24s %s%s\n",
                    s.name.toRawUTF8(),
                    s.description.toRawUTF8(),
                    s.requiresTestHooks ? "  [requires STORYBORED_TEST_HOOKS]" : "");
    }
}

// Looks up `opt` accepting either `--opt value` or `--opt=value`. Returns "" if not present.
juce::String getOpt(int argc, char** argv, const char* opt)
{
    const juce::String optStr(opt);
    const juce::String optEq = optStr + "=";
    for (int i = 1; i < argc; ++i) {
        juce::String a(argv[i]);
        if (a == optStr) {
            if (i + 1 < argc && ! juce::String(argv[i + 1]).startsWith("--"))
                return juce::String(argv[i + 1]);
            return "<flag>";
        }
        if (a.startsWith(optEq))
            return a.substring(optEq.length());
    }
    return {};
}

bool hasFlag(int argc, char** argv, const char* opt)
{
    const juce::String optStr(opt);
    for (int i = 1; i < argc; ++i)
        if (juce::String(argv[i]) == optStr)
            return true;
    return false;
}

int realMain(int argc, char** argv)
{
    CrashReporter::install();

    if (hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h")) { printUsage(); return 0; }
    if (hasFlag(argc, argv, "--version"))                            { std::puts(HARNESS_VERSION_STRING); return 0; }
    if (hasFlag(argc, argv, "--list-scenarios"))                     { listScenarios(); return 0; }

    HarnessConfig cfg;
    Logger::setVerbose(hasFlag(argc, argv, "--verbose") || hasFlag(argc, argv, "-v"));
    cfg.verbose    = Logger::isVerbose();
    cfg.withEditor = hasFlag(argc, argv, "--with-editor");

    if (hasFlag(argc, argv, "--list-formats")) {
        PluginLoader pl;
        for (const auto& f : pl.availableFormats())
            std::puts(f.toRawUTF8());
        return 0;
    }

    const auto setStr = [&](juce::String& dst, const char* opt) {
        auto v = getOpt(argc, argv, opt);
        if (v.isNotEmpty() && v != "<flag>") dst = v;
    };
    const auto setInt = [&](int& dst, const char* opt) {
        auto v = getOpt(argc, argv, opt);
        if (v.isNotEmpty() && v != "<flag>") dst = v.getIntValue();
    };

    setStr(cfg.pluginPath,   "--plugin");
    setStr(cfg.format,       "--format");
    setStr(cfg.scenario,     "--scenario");
    setStr(cfg.hostName,     "--host-name");
    setStr(cfg.junitOut,     "--junit-out");
    setInt(cfg.instances,    "--instances");
    setInt(cfg.durationSecs, "--duration");
    setInt(cfg.iterations,   "--iterations");
    setInt(cfg.blockSize,    "--block-size");
    {
        auto v = getOpt(argc, argv, "--sample-rate");
        if (v.isNotEmpty() && v != "<flag>") cfg.sampleRate = v.getDoubleValue();
    }

    {
        auto v = getOpt(argc, argv, "--seed");
        if (v.isNotEmpty() && v != "<flag>") {
            cfg.seed         = static_cast<uint64_t>(v.getLargeIntValue());
            cfg.seedExplicit = true;
        } else {
            std::random_device rd;
            cfg.seed = (static_cast<uint64_t>(rd()) << 32) | rd();
        }
    }

    if (cfg.scenario.isEmpty()) {
        std::fputs("error: --scenario is required (use --list-scenarios)\n", stderr);
        return 2;
    }
    if (cfg.pluginPath.isEmpty()) {
        std::fputs("error: --plugin is required\n", stderr);
        return 2;
    }

    auto scenario = ScenarioFactory::create(cfg.scenario);
    if (scenario == nullptr) {
        std::fprintf(stderr, "error: unknown scenario '%s'\n", cfg.scenario.toRawUTF8());
        return 2;
    }

    Harness harness(cfg);
    return harness.run(*scenario);
}

} // namespace
} // namespace sbharness

int main(int argc, char** argv)
{
    juce::MessageManager::getInstance();
    const int rc = sbharness::realMain(argc, argv);
    juce::MessageManager::deleteInstance();
    juce::DeletedAtShutdown::deleteAll();
    return rc;
}
