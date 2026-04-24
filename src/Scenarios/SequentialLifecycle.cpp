#include "../Harness.h"
#include "../Logging.h"
#include "../Scenario.h"

namespace sbharness {

namespace {
class SequentialLifecycle : public Scenario {
public:
    juce::String name() const override { return "sequential-lifecycle"; }

    int run(Harness& h) override
    {
        const int iterations = juce::jmax(1, h.config().iterations);
        int failures = 0;

        for (int i = 0; i < iterations; ++i) {
            auto inst = h.createInstance();
            if (inst == nullptr) { ++failures; continue; }

            h.prepare(*inst);
            h.processFor(*inst, 100);
            inst->releaseResources();
            inst.reset();

            if ((i + 1) % 10 == 0)
                Logger::debug("sequential-lifecycle iter " + juce::String(i + 1) + "/"
                              + juce::String(iterations));
        }
        return failures;
    }
};
}

std::unique_ptr<Scenario> makeSequentialLifecycle()
{
    return std::make_unique<SequentialLifecycle>();
}

} // namespace sbharness
