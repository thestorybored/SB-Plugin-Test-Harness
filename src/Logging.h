#pragma once

#include <juce_core/juce_core.h>
#include <iosfwd>

namespace sbharness {

enum class LogLevel { Debug, Info, Warn, Error };

class Logger {
public:
    static void setVerbose(bool verbose);
    static void setStream(std::ostream* stream);    // for tests
    static bool isVerbose();

    static void log(LogLevel level, const juce::String& msg);
    static void debug(const juce::String& msg) { log(LogLevel::Debug, msg); }
    static void info (const juce::String& msg) { log(LogLevel::Info,  msg); }
    static void warn (const juce::String& msg) { log(LogLevel::Warn,  msg); }
    static void error(const juce::String& msg) { log(LogLevel::Error, msg); }
};

} // namespace sbharness
