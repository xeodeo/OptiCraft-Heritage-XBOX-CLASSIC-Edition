// Exception-dispatch probes, isolated from C++ EH in their own TU.
#include <excpt.h>
#include <stdexcept>

extern "C" void __stdcall RaiseException(unsigned long code, unsigned long flags, unsigned long nargs, const unsigned long* args);

static volatile unsigned long g_lastCode = 0;

static int Filter(unsigned long code)
{
    g_lastCode = code;
    return 1;  // EXCEPTION_EXECUTE_HANDLER
}

// 1 if a plain SEH exception is dispatched to an __except handler.
extern "C" int ProbeSeh(void)
{
    __try {
        RaiseException(0xE0001234, 0, 0, nullptr);
    } __except (Filter(GetExceptionCode())) {
        return g_lastCode == 0xE0001234 ? 1 : 0;
    }
    return 0;
}

static void ThrowCpp()
{
    throw std::runtime_error("probe");
}

// Code seen by an SEH filter when C++ throws (expect 0xE06D7363), 0 if none.
extern "C" unsigned long ProbeCppThrowCode(void)
{
    g_lastCode = 0;
    __try {
        ThrowCpp();
    } __except (Filter(GetExceptionCode())) {
    }
    return g_lastCode;
}
