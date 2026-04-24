# Monorepo integration contract

This document describes what the **StoryBored monorepo** must provide so the
harness can run useful scenarios. The harness itself doesn't implement any of
this — it only consumes the contract.

## 1. CMake option: `STORYBORED_TEST_HOOKS`

Add to the monorepo's plugin-target CMake (e.g.
`Source/MICROSYNTH/cmake/CMakeLists.txt`):

```cmake
option(STORYBORED_TEST_HOOKS
       "Expose lifecycle/licensing/sentry test hooks for the harness." OFF)

if(STORYBORED_TEST_HOOKS)
    target_compile_definitions(${plugin_target} PRIVATE STORYBORED_TEST_HOOKS=1)
endif()
```

**Default OFF.** Release-build CI must assert it stays OFF — the test-hook
symbols must never ship.

A suitable guard:

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND STORYBORED_TEST_HOOKS)
    message(FATAL_ERROR
            "STORYBORED_TEST_HOOKS=ON is forbidden in Release builds.")
endif()
```

## 2. Headless-guard bypass

In every plugin's `PluginProcessor.cpp`, the existing headless-environment
guard reads roughly:

```cpp
if (juce::PluginHostType().getHostDescription() == "Unknown")
    return;  // skip Sentry / PostHog / LemonSqueezy init
```

When `STORYBORED_TEST_HOOKS` is defined, this guard MUST be bypassed
unconditionally so the harness exercises the same singleton init paths a real
DAW would. The contractually safe pattern:

```cpp
const bool isHeadless = juce::PluginHostType().getHostDescription() == "Unknown";
#ifndef STORYBORED_TEST_HOOKS
if (isHeadless) return;
#endif
```

This couples the bypass to the test-hook build, which is what CI for this
harness uses anyway. No host-name detection trickery required.

## 3. Exported C symbol: `storybored_test_hooks_v1`

The plugin's dynamic library MUST export a single `extern "C"` factory
function the harness looks up via `dlsym` / `GetProcAddress`:

```cpp
// In a new file Source/Common/StoryBoredTestHooks.cpp, compiled only when
// STORYBORED_TEST_HOOKS is defined.

#include "StoryBoredTestHooksV1.h"   // the same struct definition the harness
                                     // ships in src/TestHookBridge.h
#include "../sentry-integration/SentryManager.h"
#include "../posthog-analytics/PostHogManager.h"
#include "../licensing-system/LemonSqueezyClient.h"

#if defined(_WIN32)
 #define STORYBORED_EXPORT __declspec(dllexport)
#else
 #define STORYBORED_EXPORT __attribute__((visibility("default")))
#endif

namespace {
    bool activate(juce::AudioPluginInstance* inst, const char* key) {
        // ... call your LicenseManager async path ...
        return true;
    }
    // ... and so on for every hook ...

    StoryBoredTestHooksV1 g_hooks_v1 = {
        /*abiVersion*/                    1,
        /*activateLicenseAsync*/          &activate,
        /*deactivateLicenseAsync*/        &deactivate,
        /*triggerAnalyticsFlush*/         &flushAnalytics,
        /*triggerSentryTestEvent*/        &sendSentryTest,
        /*getActiveSentryInstanceCount*/  &SentryManager::getActiveInstanceCount,
        /*getActivePostHogInstanceCount*/ &PostHogManager::getActiveInstanceCount,
        /*getActiveHTTPRequestCount*/     &LemonSqueezyClient::getActiveRequestCount,
    };
}

extern "C" STORYBORED_EXPORT
StoryBoredTestHooksV1* storybored_test_hooks_v1(juce::AudioPluginInstance*) {
    return &g_hooks_v1;
}
```

### Symbol visibility

The plugin's main CMake link config likely sets
`CXX_VISIBILITY_PRESET=hidden`. The function above uses `__attribute__((visibility("default")))` /
`__declspec(dllexport)` so it survives that. Also ensure the linker doesn't
strip the symbol — for VST3 bundles on macOS, the existing JUCE export list
should be augmented, or use a `LINKER:--export-dynamic-symbol=storybored_test_hooks_v1`
flag on Linux.

## 4. ABI stability

`StoryBoredTestHooksV1` is **frozen**. Adding a hook means appending a new
function pointer at the end of the struct AND adding a new
`storybored_test_hooks_v2` factory symbol. The harness pins to a specific ABI
via the `abiVersion` field and refuses to call a struct with a newer version
than it understands.

When you add a hook:

1. Add a new field at the end of the struct in `src/TestHookBridge.h` (this
   harness repo) AND in your monorepo header. **Do not reorder existing
   fields.**
2. If the hook is required (not just additive), bump to `V2`: define
   `StoryBoredTestHooksV2` containing all of `V1`'s fields plus the new ones,
   and export `storybored_test_hooks_v2`.
3. Submit the harness change as a PR; once merged, bump the harness submodule
   pin in the monorepo and roll out together.

## 5. Build integration in the monorepo

### `CMakeLists.txt` (top-level)

```cmake
option(STORYBORED_BUILD_HARNESS "Build the SB-689 lifecycle test harness" OFF)
if(STORYBORED_BUILD_HARNESS)
    add_subdirectory(external/SB-Plugin-Test-Harness)
endif()
```

### `build.sh`

Add a `--build-harness` flag:

```bash
EXTRA_CMAKE_FLAGS=()
case "$arg" in
    --build-harness)
        EXTRA_CMAKE_FLAGS+=(-DSTORYBORED_BUILD_HARNESS=ON
                            -DSTORYBORED_TEST_HOOKS=ON
                            -DHARNESS_ENABLE_ASAN=ON)
        ;;
esac
```

### CI

```bash
./build.sh --resynth-only --debug --build-harness
./build-debug/external/SB-Plugin-Test-Harness/plugin-harness_artefacts/Debug/plugin-harness \
    --plugin ~/Library/Audio/Plug-Ins/VST3/reSYNTH.vst3 \
    --format VST3 \
    --scenario staggered-teardown \
    --instances 8 \
    --junit-out harness-results.xml
```

Failure of any scenario fails CI.

## 6. Verification checklist

- [ ] `STORYBORED_TEST_HOOKS` CMake option lands in plugin CMake.
- [ ] Release-build guard added.
- [ ] Headless-guard bypass added under `#ifndef STORYBORED_TEST_HOOKS`.
- [ ] `storybored_test_hooks_v1` exported with correct visibility.
- [ ] `nm -D` (Linux) / `dumpbin /exports` (Windows) / `nm -gU` (macOS) shows
      the symbol on test-hook builds, absent on release builds.
- [ ] Harness `mid-request-teardown` scenario logs
      `TestHookBridge: hooks v1 available` instead of "not exported by plugin".
