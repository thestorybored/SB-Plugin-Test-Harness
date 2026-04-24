#include "../Harness.h"
#include "../Logging.h"
#include "../Scenario.h"

#include <latch>
#include <thread>
#include <vector>

namespace sbharness {

namespace {
class ParallelTeardown : public Scenario {
public:
    juce::String name() const override { return "parallel-teardown"; }

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
        for (auto& inst : instances)
            if (inst) h.processFor(*inst, 200);

        std::latch start_gate(1);
        std::vector<std::thread> threads;
        threads.reserve(instances.size());

        for (auto& slot : instances) {
            auto* raw = slot.release();
            threads.emplace_back([raw, &start_gate]() {
                start_gate.wait();
                std::unique_ptr<juce::AudioPluginInstance> own(raw);
                if (own) own->releaseResources();
                own.reset();
            });
        }

        Logger::info("parallel-teardown: releasing latch with "
                     + juce::String(threads.size()) + " threads");
        start_gate.count_down();

        for (auto& t : threads) t.join();
        return failures;
    }
};
}

std::unique_ptr<Scenario> makeParallelTeardown()
{
    return std::make_unique<ParallelTeardown>();
}

} // namespace sbharness
