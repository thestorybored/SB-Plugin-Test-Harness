#include "Logging.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace sbharness {

namespace {
    std::mutex g_mutex;
    bool g_verbose = false;
    std::ostream* g_stream = nullptr;

    const char* levelTag(LogLevel level)
    {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO ";
            case LogLevel::Warn:  return "WARN ";
            case LogLevel::Error: return "ERROR";
        }
        return "?????";
    }

    juce::String timestamp()
    {
        const auto now = std::chrono::system_clock::now();
        const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch()).count() % 1000;
        const auto t   = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        gmtime_s(&tm, &t);
#else
        gmtime_r(&t, &tm);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0') << ms << 'Z';
        return juce::String(oss.str());
    }
}

void Logger::setVerbose(bool verbose) { g_verbose = verbose; }
void Logger::setStream(std::ostream* stream) { g_stream = stream; }
bool Logger::isVerbose() { return g_verbose; }

void Logger::log(LogLevel level, const juce::String& msg)
{
    if (level == LogLevel::Debug && ! g_verbose)
        return;

    std::lock_guard<std::mutex> lk(g_mutex);
    auto& out = (g_stream != nullptr) ? *g_stream : std::cerr;
    out << timestamp() << ' ' << levelTag(level) << ' '
        << msg.toStdString() << '\n';
    out.flush();
}

} // namespace sbharness
