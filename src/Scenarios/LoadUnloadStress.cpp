#include "../Harness.h"
#include "../Logging.h"
#include "../Scenario.h"

namespace sbharness {

namespace {
class LoadUnloadStress : public Scenario {
public:
    juce::String name() const override { return "load-unload-stress"; }

    int run(Harness& h) override
    {
        const int iterations = juce::jmax(1, h.config().iterations);
        int failures = 0;

        for (int i = 0; i < iterations; ++i) {
            auto inst = h.createInstance();
            if (inst == nullptr) { ++failures; continue; }
            h.prepare(*inst);
            inst->releaseResources();
            inst.reset();
        }
        return failures;
    }
};
}

std::unique_ptr<Scenario> makeLoadUnloadStress()
{
    return std::make_unique<LoadUnloadStress>();
}

} // namespace sbharness
