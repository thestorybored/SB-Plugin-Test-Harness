#include "TestHookBridge.h"

#include "Logging.h"

#if JUCE_WINDOWS
 #include <windows.h>
#else
 #include <dlfcn.h>
#endif

namespace sbharness {

namespace {
    constexpr const char* kFactorySymbol = "storybored_test_hooks_v1";

    // Returns the actual loadable binary inside a plugin bundle/dir.
    // VST3 on macOS:   Foo.vst3/Contents/MacOS/Foo
    // VST3 on Linux:   Foo.vst3/Contents/x86_64-linux/Foo.so
    // VST3 on Windows: Foo.vst3/Contents/x86_64-win/Foo.vst3 (or single-file .vst3)
    // AU:              Foo.component/Contents/MacOS/Foo
    // CLAP:            Foo.clap (single file on all platforms)
    juce::File resolveLoadableBinary(const juce::String& pluginPath)
    {
        juce::File p(pluginPath);
        if (! p.exists()) return p;
        if (p.existsAsFile()) return p;     // single-file (CLAP, Win VST3 single-file)

        // Bundle/directory: dive into Contents/.
        auto contents = p.getChildFile("Contents");
        if (! contents.isDirectory()) return p;

       #if JUCE_MAC || JUCE_IOS
        auto macos = contents.getChildFile("MacOS");
        if (macos.isDirectory()) {
            auto bin = macos.getChildFile(p.getFileNameWithoutExtension());
            if (bin.existsAsFile()) return bin;
            // Fallback: first executable in MacOS/.
            auto entries = macos.findChildFiles(juce::File::findFiles, false);
            if (! entries.isEmpty()) return entries.getFirst();
        }
       #elif JUCE_LINUX
        auto archDir = contents.getChildFile("x86_64-linux");
        if (archDir.isDirectory()) {
            auto entries = archDir.findChildFiles(juce::File::findFiles, false, "*.so");
            if (! entries.isEmpty()) return entries.getFirst();
        }
       #elif JUCE_WINDOWS
        for (auto sub : { "x86_64-win", "x86-win" }) {
            auto archDir = contents.getChildFile(sub);
            if (archDir.isDirectory()) {
                auto entries = archDir.findChildFiles(juce::File::findFiles, false);
                if (! entries.isEmpty()) return entries.getFirst();
            }
        }
       #endif
        return p;
    }
}

TestHookBridge::TestHookBridge() = default;

TestHookBridge::~TestHookBridge()
{
    if (dsoHandle != nullptr) {
       #if JUCE_WINDOWS
        FreeLibrary(static_cast<HMODULE>(dsoHandle));
       #else
        dlclose(dsoHandle);
       #endif
    }
}

bool TestHookBridge::openFromPluginPath(const juce::String& pluginPath)
{
    auto bin = resolveLoadableBinary(pluginPath);
    if (! bin.existsAsFile()) {
        Logger::warn("TestHookBridge: no loadable binary at " + pluginPath);
        return false;
    }

    const auto binStr = bin.getFullPathName().toStdString();

   #if JUCE_WINDOWS
    auto handle = LoadLibraryA(binStr.c_str());
    if (handle == nullptr) {
        Logger::warn("TestHookBridge: LoadLibrary failed for " + bin.getFullPathName());
        return false;
    }
    auto sym = GetProcAddress(handle, kFactorySymbol);
    dsoHandle = handle;
   #else
    auto* handle = dlopen(binStr.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (handle == nullptr) {
        const char* err = dlerror();
        Logger::warn(juce::String("TestHookBridge: dlopen failed: ")
                     + juce::String(err != nullptr ? err : "(no error)"));
        return false;
    }
    auto* sym = dlsym(handle, kFactorySymbol);
    dsoHandle = handle;
   #endif

    if (sym == nullptr) {
        Logger::info(juce::String("TestHookBridge: ") + juce::String(kFactorySymbol)
                     + " not exported by plugin (built without STORYBORED_TEST_HOOKS?)");
        factory = nullptr;
        return false;
    }
    factory = reinterpret_cast<Factory>(sym);
    Logger::info("TestHookBridge: hooks v1 available");
    return true;
}

StoryBoredTestHooksV1* TestHookBridge::get(juce::AudioPluginInstance& inst)
{
    if (factory == nullptr) return nullptr;
    auto* hooks = factory(&inst);
    if (hooks != nullptr && hooks->abiVersion != 1) {
        Logger::error("TestHookBridge: hook ABI mismatch (got "
                      + juce::String(hooks->abiVersion) + ", expected 1)");
        return nullptr;
    }
    return hooks;
}

} // namespace sbharness
