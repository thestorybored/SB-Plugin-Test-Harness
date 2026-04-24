# SB-Plugin-Test-Harness

A small JUCE-based C++ test harness that loads StoryBored audio plugins
(VST3 / AU / CLAP) into a single host process, runs configurable
multi-instance lifecycle scenarios, and reports crashes and AddressSanitizer
hits in a CI-friendly way.

The bug class this is designed to catch is the SB-689 family: a destructor on
one instance tearing down a process-wide singleton (Sentry, PostHog,
Lemon Squeezy HTTP client) while another instance still has callbacks pending.
Tools like `pluginval` trip the plugin's headless-environment guard and never
exercise the buggy code; this harness identifies as a real host so the
networking init runs.

## Build

```bash
git clone https://github.com/thestorybored/SB-Plugin-Test-Harness.git
cd SB-Plugin-Test-Harness
git submodule update --init --recursive

# Debug + ASan + UBSan (recommended for SB-689-class bugs):
cmake -S . -B build-asan -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DHARNESS_ENABLE_ASAN=ON \
    -DHARNESS_BUILD_TESTS=ON
cmake --build build-asan -j

# Release:
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Required system packages (Linux only)

```bash
sudo apt-get install -y --no-install-recommends \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxcomposite-dev \
    libasound2-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev
```

macOS and Windows have no extra system deps beyond Xcode / MSVC.

## Run

```bash
./build-asan/plugin-harness_artefacts/Debug/plugin-harness \
    --plugin /path/to/reSYNTH.vst3 \
    --format VST3 \
    --scenario staggered-teardown \
    --instances 8 \
    --duration 60 \
    --sample-rate 48000 \
    --block-size 512 \
    --seed 42 \
    --junit-out results.xml \
    --verbose
```

Or use `scripts/run-local.sh`, which sets sane `ASAN_OPTIONS` defaults and
finds the binary automatically:

```bash
scripts/run-local.sh /path/to/reSYNTH.vst3 staggered-teardown --instances 8
```

Wrap with `scripts/collect-results.py` to capture crash stacks into JUnit when
the binary exits abnormally:

```bash
scripts/collect-results.py --junit-out results.xml -- \
    ./build-asan/plugin-harness_artefacts/Debug/plugin-harness \
    --plugin reSYNTH.vst3 --scenario staggered-teardown --instances 8
```

## Exit codes

| code  | meaning                                                          |
|-------|------------------------------------------------------------------|
| 0     | scenario completed, no failures                                  |
| 1     | scenario completed but reported assertion / leak failures        |
| 2     | bad CLI arguments                                                |
| 134   | SIGABRT (e.g. ASan detected a violation, or assertion abort)     |
| 139   | SIGSEGV                                                          |
| other | OS-level signal; `scripts/collect-results.py` synthesizes a JUnit case |

## Scenarios

```
sequential-lifecycle      Serial: instantiate -> process -> destroy, repeat.
interleaved-lifecycle     Hold N instances; on each tick, swap a random one.
staggered-teardown        Create N instances; destroy in shuffled order with gaps.
                          (Direct SB-689 repro.)
parallel-teardown         Create N instances; destroy all from N threads at once
                          (synchronized via std::latch).
mid-request-teardown      Trigger HTTP request via test hook; destroy mid-flight.
                          [requires STORYBORED_TEST_HOOKS=ON plugin build]
long-idle-batch-flush     Idle past PostHog batch-flush interval (65s); destroy.
load-unload-stress        Create+destroy a single instance as fast as possible.
```

`plugin-harness --list-scenarios` is the source of truth.

## Adding a scenario

1. Create `src/Scenarios/YourScenario.cpp`. Implement `Scenario`:
   ```cpp
   #include "../Harness.h"
   #include "../Scenario.h"
   namespace sbharness {
   namespace {
   class YourScenario : public Scenario {
       juce::String name() const override { return "your-scenario"; }
       int run(Harness& h) override {
           // ... use h.createInstance(), h.prepare(...), h.processFor(...),
           //     h.rng() for seeded RNG.
           return /* failure count */ 0;
       }
   };
   }
   std::unique_ptr<Scenario> makeYourScenario() {
       return std::make_unique<YourScenario>();
   }
   } // namespace sbharness
   ```
2. Add the source file to `target_sources(plugin-harness ...)` in
   `CMakeLists.txt`.
3. Forward-declare `makeYourScenario()` and add an entry to the registry in
   `src/Scenario.cpp`.

A scenario should fit in under 100 lines of new code.

## Adding a test hook

Test hooks live in the StoryBored monorepo (this harness only consumes them).
The contract is documented in [docs/MONOREPO_INTEGRATION.md](docs/MONOREPO_INTEGRATION.md).

In short:

1. Add a new function pointer to `StoryBoredTestHooksV1` in
   `src/TestHookBridge.h`.
2. Update the corresponding implementation in the StoryBored monorepo so the
   exported `storybored_test_hooks_v1` returns a struct with the new pointer
   populated. Bump the ABI version if the new field is required.
3. Use the new hook from a scenario via `h.hooks().get(*instance)->yourHook(...)`.

## JUnit output

`--junit-out PATH` writes a JUnit-style XML report:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name="plugin-harness" tests="1" failures="0" errors="0" time="0.123">
    <testcase classname="harness" name="staggered-teardown" time="0.123"/>
  </testsuite>
</testsuites>
```

If the binary crashes (exit code >= 128 or signal), `scripts/collect-results.py`
synthesizes a `<testcase>` with a `<failure>` and an embedded `<system-err>`
containing the captured stderr tail.

## Running under ASan locally

Sanitizer flags are configured automatically when building with
`-DHARNESS_ENABLE_ASAN=ON`. Recommended runtime options:

```bash
export ASAN_OPTIONS=abort_on_error=1:symbolize=1:print_stacktrace=1:detect_leaks=0
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1
```

Both are set automatically by `scripts/run-local.sh`.

## Integrating into the StoryBored monorepo

Add as a Git submodule:

```bash
cd $STORYBORED_ROOT
git submodule add git@github.com:thestorybored/SB-Plugin-Test-Harness.git \
    external/SB-Plugin-Test-Harness
git submodule update --init --recursive
```

Add to the monorepo's top-level `CMakeLists.txt`:

```cmake
option(STORYBORED_BUILD_HARNESS "Build the SB-689 lifecycle test harness" OFF)
if(STORYBORED_BUILD_HARNESS)
    add_subdirectory(external/SB-Plugin-Test-Harness)
endif()
```

Add a `--build-harness` flag to `build.sh` that passes
`-DSTORYBORED_BUILD_HARNESS=ON -DSTORYBORED_TEST_HOOKS=ON` through to CMake,
then expose the binary at `build-debug/external/SB-Plugin-Test-Harness/plugin-harness_artefacts/Debug/plugin-harness`.

The plugin side of the contract (the `STORYBORED_TEST_HOOKS` CMake option,
the exported `storybored_test_hooks_v1` C symbol, the headless-guard bypass)
is documented in
[docs/MONOREPO_INTEGRATION.md](docs/MONOREPO_INTEGRATION.md).

## License

MIT. See [LICENSE](LICENSE).
