#include "JUnitReport.h"

#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>

TEST_CASE("JUnitReport writes valid XML for a passing case", "[junit]")
{
    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("harness-junit-pass.xml");
    tmp.deleteFile();

    sbharness::JUnitReport r("plugin-harness", tmp.getFullPathName());
    sbharness::TestCase tc;
    tc.classname    = "harness";
    tc.name         = "smoke";
    tc.durationSecs = 0.123;
    r.addCase(std::move(tc));

    REQUIRE(r.write());
    REQUIRE(tmp.existsAsFile());

    const auto contents = tmp.loadFileAsString();
    REQUIRE(contents.contains("<testsuite"));
    REQUIRE(contents.contains("name=\"plugin-harness\""));
    REQUIRE(contents.contains("name=\"smoke\""));
    REQUIRE(! contents.contains("<failure"));
    REQUIRE(r.failureCount() == 0);

    tmp.deleteFile();
}

TEST_CASE("JUnitReport escapes failure messages and counts failures", "[junit]")
{
    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("harness-junit-fail.xml");
    tmp.deleteFile();

    sbharness::JUnitReport r("plugin-harness", tmp.getFullPathName());
    sbharness::TestCase tc;
    tc.classname       = "harness";
    tc.name            = "boom";
    tc.failureMessage  = "expected <X> got \"Y\" & 'Z'";
    r.addCase(std::move(tc));

    REQUIRE(r.write());
    REQUIRE(r.failureCount() == 1);
    const auto contents = tmp.loadFileAsString();
    REQUIRE(contents.contains("<failure"));
    REQUIRE(contents.contains("&lt;X&gt;"));
    REQUIRE(contents.contains("&quot;Y&quot;"));
    REQUIRE(contents.contains("&apos;Z&apos;"));
    REQUIRE(contents.contains("&amp;"));

    tmp.deleteFile();
}

TEST_CASE("JUnitReport with empty path is a no-op", "[junit]")
{
    sbharness::JUnitReport r("plugin-harness", "");
    REQUIRE(r.write());
    REQUIRE(r.caseCount() == 0);
}
