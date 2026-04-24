#include "../Harness.h"
#include "../Logging.h"
#include "../Scenario.h"

#include <chrono>
#include <thread>
#include <vector>

namespace sbharness {

namespace {
// Idle past PostHog batch-flush interval (60 s + slack), then destroy.
// Catches singleton + detached flush-thread lifecycle bugs.
class LongIdleBatchFlush : public Scenario {
public:
    juce::String name() const override { return "long-idle-batch-flush"; }

    int run(Harness& h) override
    {
        const int n = 2;
        int failures = 0;

        std::vector<std::unique_ptr<juce::AudioPluginInstance>> instances;
        instances.reserve(n);
        for (int i = 0; i < n; ++i) {
            auto inst = h.createInstance();
            if (inst == nullptr) { ++failures; continue; }
            h.prepare(*inst);
            instances.push_back(std::move(inst));
        }

        // duration override: respect --duration if >= 65, otherwise default 65s.
        const int idleSecs = juce::jmax(65, h.config().durationSecs);
        Logger::info("long-idle-batch-flush: idling " + juce::String(idleSecs) + "s");

        const auto deadline = std::chrono::steady_clock::now()
                              + std::chrono::seconds(idleSecs);
        while (std::chrono::steady_clock::now() < deadline) {
            for (auto& inst : instances)
                if (inst) h.processBlocks(*inst, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        for (auto& inst : instances) {
            if (! inst) continue;
            inst->releaseResources();
            inst.reset();
        }
        return failures;
    }
};
}

std::unique_ptr<Scenario> makeLongIdleBatchFlush()
{
    return std::make_unique<LongIdleBatchFlush>();
}

} // namespace sbharness
