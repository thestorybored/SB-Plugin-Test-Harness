include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(harness-self-tests
    HarnessSelfTest.cpp
    JUnitReportSelfTest.cpp
    ../src/Logging.cpp
    ../src/JUnitReport.cpp)

target_include_directories(harness-self-tests PRIVATE ../src)
target_compile_definitions(harness-self-tests PRIVATE
    JUCE_DISABLE_JUCE_VERSION_PRINTING=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0)

target_link_libraries(harness-self-tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_core
    juce::juce_recommended_config_flags)

harness_apply_sanitizers(harness-self-tests)

include(Catch)
catch_discover_tests(harness-self-tests)
