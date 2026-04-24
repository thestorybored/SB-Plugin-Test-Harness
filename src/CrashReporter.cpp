#include "CrashReporter.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <initializer_list>

#if defined(_WIN32)
 #include <windows.h>
 #include <dbghelp.h>
 #pragma comment(lib, "dbghelp.lib")
#else
 #include <csignal>
 #if __has_include(<execinfo.h>)
  #include <execinfo.h>
  #define HARNESS_HAS_EXECINFO 1
 #endif
 #include <unistd.h>
#endif

namespace sbharness {

namespace {
    bool g_installed = false;

#if defined(_WIN32)
    LONG WINAPI sehHandler(EXCEPTION_POINTERS* info)
    {
        std::fputs("\n=== plugin-harness CRASH ===\n", stderr);
        std::fprintf(stderr, "exception code: 0x%08lx\n",
                     info->ExceptionRecord->ExceptionCode);

        void* frames[64];
        const auto count = CaptureStackBackTrace(0, 64, frames, nullptr);
        const auto proc  = GetCurrentProcess();
        SymInitialize(proc, nullptr, TRUE);

        char buf[sizeof(SYMBOL_INFO) + 256];
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
        sym->MaxNameLen   = 255;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        for (USHORT i = 0; i < count; ++i) {
            DWORD64 disp = 0;
            if (SymFromAddr(proc, reinterpret_cast<DWORD64>(frames[i]), &disp, sym))
                std::fprintf(stderr, "  #%u %s\n", i, sym->Name);
            else
                std::fprintf(stderr, "  #%u 0x%p\n", i, frames[i]);
        }
        std::fflush(stderr);
        return EXCEPTION_CONTINUE_SEARCH;
    }
#else
    void signalHandler(int sig)
    {
        const char* name = strsignal(sig);
        std::fputs("\n=== plugin-harness CRASH ===\n", stderr);
        std::fprintf(stderr, "signal: %d (%s)\n", sig, name ? name : "?");
       #if HARNESS_HAS_EXECINFO
        void* frames[64];
        const int n = backtrace(frames, 64);
        backtrace_symbols_fd(frames, n, fileno(stderr));
       #endif
        std::fflush(stderr);

        // Re-raise default handler so OS produces a core / ASan still reports.
        std::signal(sig, SIG_DFL);
        std::raise(sig);
    }
#endif
}

void CrashReporter::install()
{
    if (g_installed) return;
    g_installed = true;

#if defined(_WIN32)
    SetUnhandledExceptionFilter(&sehHandler);
#else
    struct sigaction sa{};
    sa.sa_handler = &signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER | SA_RESETHAND;
    for (int s : {SIGSEGV, SIGBUS, SIGABRT, SIGILL, SIGFPE})
        sigaction(s, &sa, nullptr);
#endif
}

} // namespace sbharness
