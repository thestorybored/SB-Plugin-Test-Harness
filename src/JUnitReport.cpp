#include "JUnitReport.h"

#include "Logging.h"

#include <sstream>

namespace sbharness {

namespace {
    juce::String xmlEscape(const juce::String& s)
    {
        return s.replace("&", "&amp;")
                .replace("<", "&lt;")
                .replace(">", "&gt;")
                .replace("\"", "&quot;")
                .replace("'", "&apos;");
    }
}

JUnitReport::JUnitReport(juce::String suite, juce::String out)
    : suiteName(std::move(suite)), outPath(std::move(out)) {}

void JUnitReport::addCase(TestCase c) { cases.push_back(std::move(c)); }

int JUnitReport::failureCount() const
{
    int n = 0;
    for (const auto& c : cases)
        if (c.failureMessage.isNotEmpty()) ++n;
    return n;
}

bool JUnitReport::write() const
{
    if (outPath.isEmpty())
        return true;

    double totalTime = 0.0;
    for (const auto& c : cases) totalTime += c.durationSecs;

    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<testsuites>\n"
        << "  <testsuite name=\"" << xmlEscape(suiteName).toStdString() << "\""
        << " tests=\"" << cases.size() << "\""
        << " failures=\"" << failureCount() << "\""
        << " errors=\"0\""
        << " time=\"" << totalTime << "\">\n";

    for (const auto& c : cases) {
        xml << "    <testcase classname=\"" << xmlEscape(c.classname).toStdString() << "\""
            << " name=\"" << xmlEscape(c.name).toStdString() << "\""
            << " time=\"" << c.durationSecs << "\">\n";
        if (c.failureMessage.isNotEmpty()) {
            xml << "      <failure message=\""
                << xmlEscape(c.failureMessage).toStdString() << "\"/>\n";
        }
        if (c.stdoutCapture.isNotEmpty()) {
            xml << "      <system-out>" << xmlEscape(c.stdoutCapture).toStdString()
                << "</system-out>\n";
        }
        if (c.stderrCapture.isNotEmpty()) {
            xml << "      <system-err>" << xmlEscape(c.stderrCapture).toStdString()
                << "</system-err>\n";
        }
        xml << "    </testcase>\n";
    }
    xml << "  </testsuite>\n</testsuites>\n";

    juce::File f(outPath);
    f.getParentDirectory().createDirectory();
    if (! f.replaceWithText(juce::String(xml.str()))) {
        Logger::error("Failed to write JUnit XML to " + outPath);
        return false;
    }
    Logger::info("Wrote JUnit XML to " + outPath);
    return true;
}

} // namespace sbharness
