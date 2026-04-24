#include "../Harness.h"
#include "../Logging.h"
#include "../Scenario.h"

#include <chrono>
#include <thread>

namespace sbharness {

namespace {
// Direct repro of SB-689's licensing-HTTP path. Requires STORYBORED_TEST_HOOKS=ON
// plugin build (otherwise scenario auto-skips via Scenario::requiresTestHooks()).
class MidRequestTeardown : public Scenario {
public:
    juce::String name() const override { return "mid-request-teardown"; }
    bool requiresTestHooks() const override { return true; }

    int run(Harness& h) override
    {
        const int iterations = juce::jmax(1, h.config().iterations);
        int failures = 0;

        for (int i = 0; i < iterations; ++i) {
            auto inst = h.createInstance();
            if (inst == nullptr) { ++failures; continue; }
            h.prepare(*inst);

            auto* hooks = h.hooks().get(*inst);
            if (hooks == nullptr) {
                Logger::warn("mid-request-teardown: hooks unavailable for instance, skipping iter");
                inst.reset();
                continue;
            }

            const int requestsBefore = hooks->getActiveHTTPRequestCount();
            const bool kicked = hooks->activateLicenseAsync(inst.get(),
                                                            "TEST-HARNESS-INVALID-KEY");
            if (! kicked) {
                Logger::warn("mid-request-teardown: activateLicenseAsync returned false");
                inst.reset();
                continue;
            }

            // Tear down before the HTTP callback can fire.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            inst->releaseResources();
            inst.reset();

            // Sanity-check the singleton drained.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            const int requestsAfter = hooks->getActiveHTTPRequestCount();
            if (requestsAfter > requestsBefore) {
                Logger::warn("mid-request-teardown: leaked HTTP request after teardown ("
                             + juce::String(requestsAfter - requestsBefore) + ")");
                ++failures;
            }
        }
        return failures;
    }
};
}

std::unique_ptr<Scenario> makeMidRequestTeardown()
{
    return std::make_unique<MidRequestTeardown>();
}

} // namespace sbharness
