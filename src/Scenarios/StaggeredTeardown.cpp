#include "../Harness.h"
#include "../Logging.h"
#include "../Scenario.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace sbharness {

namespace {
// SB-689 repro: create N instances; let them all run audio for a moment;
// then destroy them in a randomised order with randomised 0-50 ms gaps.
class StaggeredTeardown : public Scenario {
public:
    juce::String name() const override { return "staggered-teardown"; }

    int run(Harness& h) override
    {
        const int n = juce::jmax(2, h.config().instances);
        int failures = 0;

        std::vector<std::unique_ptr<juce::AudioPluginInstance>> instances;
        instances.reserve(static_cast<size_t>(n));

        for (int i = 0; i < n; ++i) {
            auto inst = h.createInstance();
            if (inst == nullptr) { ++failures; continue; }
            h.prepare(*inst);
            instances.push_back(std::move(inst));
        }
        Logger::info("staggered-teardown: created " + juce::String(instances.size())
                     + " instances");

        for (auto& inst : instances)
            if (inst) h.processFor(*inst, 500);

        std::vector<size_t> order(instances.size());
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), h.rng());

        std::uniform_int_distribution<int> gap(0, 50);
        for (size_t idx : order) {
            const int delay = gap(h.rng());
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            if (instances[idx]) {
                instances[idx]->releaseResources();
                instances[idx].reset();
                Logger::debug("staggered-teardown: destroyed idx " + juce::String(idx)
                              + " after " + juce::String(delay) + " ms");
            }
        }
        return failures;
    }
};
}

std::unique_ptr<Scenario> makeStaggeredTeardown()
{
    return std::make_unique<StaggeredTeardown>();
}

} // namespace sbharness
