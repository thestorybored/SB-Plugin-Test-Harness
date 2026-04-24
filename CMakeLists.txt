cmake_minimum_required(VERSION 3.22)
project(SBPluginTestHarness
        VERSION 0.1.0
        DESCRIPTION "StoryBored multi-instance plugin lifecycle test harness"
        LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Sanitizers)
include(HarnessWarnings)

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(HARNESS_ENABLE_ASAN   "Enable AddressSanitizer + UBSan" OFF)
option(HARNESS_ENABLE_TSAN   "Enable ThreadSanitizer" OFF)
option(HARNESS_BUILD_TESTS   "Build harness self-tests (Catch2)" OFF)
option(HARNESS_WERROR        "Treat warnings as errors" OFF)

if(HARNESS_ENABLE_ASAN AND HARNESS_ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be combined. Pick one.")
endif()

# -----------------------------------------------------------------------------
# JUCE
# -----------------------------------------------------------------------------
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE submodule not found at external/JUCE. "
        "Run: git submodule update --init --recursive")
endif()

add_subdirectory(external/JUCE EXCLUDE_FROM_ALL)

# -----------------------------------------------------------------------------
# plugin-harness binary
# -----------------------------------------------------------------------------
juce_add_console_app(plugin-harness
    PRODUCT_NAME "plugin-harness"
    COMPANY_NAME "StoryBored")

target_sources(plugin-harness PRIVATE
    src/main.cpp
    src/Logging.cpp
    src/JUnitReport.cpp
    src/PluginLoader.cpp
    src/Harness.cpp
    src/Scenario.cpp
    src/TestHookBridge.cpp
    src/CrashReporter.cpp
    src/Scenarios/SequentialLifecycle.cpp
    src/Scenarios/InterleavedLifecycle.cpp
    src/Scenarios/StaggeredTeardown.cpp
    src/Scenarios/ParallelTeardown.cpp
    src/Scenarios/MidRequestTeardown.cpp
    src/Scenarios/LongIdleBatchFlush.cpp
    src/Scenarios/LoadUnloadStress.cpp)

target_include_directories(plugin-harness PRIVATE src)

target_compile_definitions(plugin-harness PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=$<IF:$<PLATFORM_ID:Darwin>,1,0>
    JUCE_PLUGINHOST_LADSPA=0
    JUCE_PLUGINHOST_LV2=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_DISABLE_JUCE_VERSION_PRINTING=1
    HARNESS_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(plugin-harness PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_core
    juce::juce_events
    juce::juce_recommended_config_flags)

if(UNIX AND NOT APPLE)
    target_link_libraries(plugin-harness PRIVATE dl)
endif()

harness_apply_sanitizers(plugin-harness)
harness_apply_warnings(plugin-harness)

# So backtrace_symbols() can resolve our own functions, not just hex offsets.
set_target_properties(plugin-harness PROPERTIES ENABLE_EXPORTS ON)

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
if(HARNESS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
