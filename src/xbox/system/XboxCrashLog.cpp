// XboxCrashLog.cpp — last words for crashes the red screen never sees.
//
// ClientPlatformPolicy::reportCrash only catches C++ exceptions that reach
// Minecraft::run on the main thread. An access violation on any thread, or a
// C++ exception escaping a worker (network reader/writer, music streamer),
// stops the console without a word: the picture freezes and the audio
// hardware loops its last buffer. This logs what happened (thread, exception
// code, address, registers and code-looking stack words for the .map) through
// the normal log, which reaches the network log, then parks the thread.
#ifdef XBOX_PLATFORM
#include "xbox/system/XboxCrashLog.h"

#include "xbox/XboxXtl.h"

#include "platform/Log.h"

#include <cstdio>
#include <exception>

namespace
{
DWORD s_mainThreadId = 0;
volatile LONG s_reported = 0;

// Addresses inside the loaded XBE image (base 0x10000). The .map lists the
// same code 0x3F0000 higher (see docs/XBOX_PORT.md, debugging).
bool looksLikeCode(DWORD value)
{
    return value >= 0x00011000 && value < 0x00800000;
}

void logStackWords(DWORD esp)
{
    char line[256];
    int length = 0;
    int found = 0;
    const DWORD* stack = reinterpret_cast<const DWORD*>(esp);
    for (int i = 0; i < 512 && found < 24; ++i)
    {
        DWORD value = 0;
        __try
        {
            value = stack[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            break;
        }
        if (!looksLikeCode(value))
            continue;
        ++found;
        length += std::snprintf(line + length, sizeof(line) - length, " %08lx", value);
        if (length > 200)
        {
            MC_LOG_ERROR("crash", "stack:%s\n", line);
            length = 0;
        }
    }
    if (length > 0)
        MC_LOG_ERROR("crash", "stack:%s\n", line);
}

[[noreturn]] void park()
{
    // Keep the thread (and with it the log sender) alive instead of letting
    // the default handler take the console down mid-sentence.
    for (;;)
        Sleep(1000);
}

LONG WINAPI unhandledFilter(EXCEPTION_POINTERS* info)
{
    const DWORD thread = GetCurrentThreadId();
    if (InterlockedExchange(&s_reported, 1) != 0)
        park();   // a second thread crashing while the first is reported
    const EXCEPTION_RECORD* record = info ? info->ExceptionRecord : nullptr;
    const CONTEXT* context = info ? info->ContextRecord : nullptr;
    MC_LOG_ERROR("crash", "unhandled exception %08lx at %p on %s thread %lu (map address %08lx)\n",
                 record ? record->ExceptionCode : 0, record ? record->ExceptionAddress : nullptr,
                 thread == s_mainThreadId ? "MAIN" : "worker", thread,
                 record ? reinterpret_cast<DWORD>(record->ExceptionAddress) + 0x3F0000 : 0);
    if (record && record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2)
        MC_LOG_ERROR("crash", "access violation: %s address %08lx\n",
                     record->ExceptionInformation[0] ? "write to" : "read of",
                     static_cast<DWORD>(record->ExceptionInformation[1]));
    if (context)
    {
        MC_LOG_ERROR("crash", "eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
                     context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi, context->Edi);
        MC_LOG_ERROR("crash", "eip=%08lx esp=%08lx ebp=%08lx\n", context->Eip, context->Esp, context->Ebp);
        logStackWords(context->Esp);
    }
    park();
}

[[noreturn]] void terminateHandler()
{
    const DWORD thread = GetCurrentThreadId();
    const char* what = "unknown";
    try
    {
        const std::exception_ptr current = std::current_exception();
        if (current)
            std::rethrow_exception(current);
        what = "std::terminate without an exception";
    }
    catch (const std::exception& e)
    {
        MC_LOG_ERROR("crash", "uncaught C++ exception on %s thread %lu: %s\n",
                     thread == s_mainThreadId ? "MAIN" : "worker", thread, e.what());
        park();
    }
    catch (...)
    {
        what = "non-std exception";
    }
    MC_LOG_ERROR("crash", "uncaught C++ exception on %s thread %lu: %s\n",
                 thread == s_mainThreadId ? "MAIN" : "worker", thread, what);
    park();
}
} // namespace

namespace XboxCrashLog
{
void install()
{
    s_mainThreadId = GetCurrentThreadId();
    SetUnhandledExceptionFilter(unhandledFilter);
    std::set_terminate(terminateHandler);
    MC_LOG_INFO("xbox", "crash log installed (main thread %lu)\n", s_mainThreadId);
}
}

#endif // XBOX_PLATFORM
