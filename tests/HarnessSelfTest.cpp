#include "Logging.h"

#include <catch2/catch_test_macros.hpp>

#include <sstream>

TEST_CASE("Logger respects verbose toggle", "[logging]")
{
    std::ostringstream out;
    sbharness::Logger::setStream(&out);

    sbharness::Logger::setVerbose(false);
    sbharness::Logger::debug("debug-suppressed");
    sbharness::Logger::info("info-shown");
    REQUIRE(out.str().find("debug-suppressed") == std::string::npos);
    REQUIRE(out.str().find("info-shown") != std::string::npos);

    out.str("");
    sbharness::Logger::setVerbose(true);
    sbharness::Logger::debug("debug-shown");
    REQUIRE(out.str().find("debug-shown") != std::string::npos);

    sbharness::Logger::setStream(nullptr);
    sbharness::Logger::setVerbose(false);
}

TEST_CASE("Logger emits levels and timestamps", "[logging]")
{
    std::ostringstream out;
    sbharness::Logger::setStream(&out);
    sbharness::Logger::setVerbose(false);

    sbharness::Logger::error("boom");
    const std::string s = out.str();
    REQUIRE(s.find("ERROR") != std::string::npos);
    REQUIRE(s.find("boom") != std::string::npos);
    // ISO-8601 prefix sanity check.
    REQUIRE(s.size() > 24);
    REQUIRE(s[4] == '-');
    REQUIRE(s[7] == '-');
    REQUIRE(s[10] == 'T');

    sbharness::Logger::setStream(nullptr);
}
