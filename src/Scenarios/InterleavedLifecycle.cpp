#include "../Harness.h"
#include "../Logging.h"
#include "../Scenario.h"

#include <chrono>
#include <thread>
#include <vector>

namespace sbharness {

namespace {
class InterleavedLifecycle : public Scenario {
public:
    juce::String name() const override { return "interleaved-lifecycle"; }

    int run(Harness& h) override
    {
        const int  pool        = juce::jmax(1, h.config().instances);
        const auto deadline    = std::chrono::steady_clock::now()
                                 + std::chrono::seconds(juce::jmax(1, h.config().durationSecs));
        int failures = 0;

        std::vector<std::unique_ptr<juce::AudioPluginInstance>> instances(pool);
        for (int i = 0; i < pool; ++i) {
            instances[i] = h.createInstance();
            if (instances[i] == nullptr) { ++failures; continue; }
            h.prepare(*instances[i]);
        }

        std::uniform_int_distribution<int> dist(0, pool - 1);
        int tick = 0;
        while (std::chrono::steady_clock::now() < deadline) {
            const int victim = dist(h.rng());
            if (instances[victim] != nullptr) {
                h.processBlocks(*instances[victim], 4);
                instances[victim]->releaseResources();
                instances[victim].reset();
            }

            auto fresh = h.createInstance();
            if (fresh == nullptr) { ++failures; continue; }
            h.prepare(*fresh);
            instances[victim] = std::move(fresh);

            if (++tick % 25 == 0)
                Logger::debug("interleaved-lifecycle ticks: " + juce::String(tick));

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return failures;
    }
};
}

std::unique_ptr<Scenario> makeInterleavedLifecycle()
{
    return std::make_unique<InterleavedLifecycle>();
}

} // namespace sbharness
