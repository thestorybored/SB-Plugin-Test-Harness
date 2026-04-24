#pragma once

#include <juce_core/juce_core.h>

#include <functional>
#include <memory>
#include <vector>

namespace sbharness {

class Harness;

class Scenario {
public:
    virtual ~Scenario() = default;
    virtual juce::String name() const = 0;
    virtual juce::String description() const { return {}; }
    virtual bool requiresTestHooks() const { return false; }

    // Returns the number of failed assertions (0 == success).
    virtual int run(Harness&) = 0;
};

namespace ScenarioFactory {
    using Factory = std::function<std::unique_ptr<Scenario>()>;

    // Returns nullptr if name is not registered.
    std::unique_ptr<Scenario> create(const juce::String& name);

    struct Entry {
        juce::String name;
        juce::String description;
        bool requiresTestHooks;
    };
    std::vector<Entry> all();
}

} // namespace sbharness
