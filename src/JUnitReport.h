#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace sbharness {

struct TestCase {
    juce::String classname;
    juce::String name;
    double durationSecs = 0.0;
    juce::String failureMessage;     // empty => passed
    juce::String stdoutCapture;
    juce::String stderrCapture;
};

class JUnitReport {
public:
    JUnitReport(juce::String suiteName, juce::String outPath);

    void addCase(TestCase c);
    bool write() const;              // returns true if path empty (no-op) or write succeeded
    int  failureCount() const;
    int  caseCount() const { return static_cast<int>(cases.size()); }

    const juce::String& path() const { return outPath; }

private:
    juce::String suiteName;
    juce::String outPath;
    std::vector<TestCase> cases;
};

} // namespace sbharness
