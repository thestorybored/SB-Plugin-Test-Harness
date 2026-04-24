#include "Scenario.h"

namespace sbharness {

// Defined in src/Scenarios/*.cpp
std::unique_ptr<Scenario> makeSequentialLifecycle();
std::unique_ptr<Scenario> makeInterleavedLifecycle();
std::unique_ptr<Scenario> makeStaggeredTeardown();
std::unique_ptr<Scenario> makeParallelTeardown();
std::unique_ptr<Scenario> makeMidRequestTeardown();
std::unique_ptr<Scenario> makeLongIdleBatchFlush();
std::unique_ptr<Scenario> makeLoadUnloadStress();

namespace {
    struct Reg {
        const char* name;
        const char* description;
        bool        requiresTestHooks;
        std::unique_ptr<Scenario> (*factory)();
    };

    const std::vector<Reg>& registry()
    {
        static const std::vector<Reg> r = {
            {"sequential-lifecycle", "Serial: instantiate -> process -> destroy, repeat.",      false, makeSequentialLifecycle},
            {"interleaved-lifecycle","Hold N instances; on each tick, swap a random one.",      false, makeInterleavedLifecycle},
            {"staggered-teardown",   "Create N instances; destroy in shuffled order with gaps.",false, makeStaggeredTeardown},
            {"parallel-teardown",    "Create N instances; destroy all from N threads at once.", false, makeParallelTeardown},
            {"mid-request-teardown", "Trigger HTTP request via test hook; destroy mid-flight.", true,  makeMidRequestTeardown},
            {"long-idle-batch-flush","Idle past PostHog batch flush interval; destroy.",        false, makeLongIdleBatchFlush},
            {"load-unload-stress",   "Create+destroy a single instance as fast as possible.",   false, makeLoadUnloadStress},
        };
        return r;
    }
}

namespace ScenarioFactory {

std::unique_ptr<Scenario> create(const juce::String& name)
{
    for (const auto& r : registry())
        if (name == r.name) return r.factory();
    return nullptr;
}

std::vector<Entry> all()
{
    std::vector<Entry> e;
    for (const auto& r : registry())
        e.push_back({juce::String(r.name), juce::String(r.description), r.requiresTestHooks});
    return e;
}

} // namespace ScenarioFactory

} // namespace sbharness
