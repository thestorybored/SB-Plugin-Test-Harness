#pragma once

namespace sbharness {

class CrashReporter {
public:
    // Installs platform-appropriate signal / SEH handlers that flush logs and
    // print a backtrace before re-raising. Idempotent.
    static void install();
};

} // namespace sbharness
