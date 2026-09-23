// Same plain-SEH probe, compiled with the XDK VC7.1 compiler (_except_handler3).
#include <excpt.h>
extern "C" void __stdcall RaiseException(unsigned long code, unsigned long flags, unsigned long nargs, const unsigned long* args);
static volatile unsigned long g_code71 = 0;
static int Filter71(unsigned long code) { g_code71 = code; return 1; }
extern "C" int ProbeSeh71(void)
{
    __try {
        RaiseException(0xE0005678, 0, 0, 0);
    } __except (Filter71(GetExceptionCode())) {
        return g_code71 == 0xE0005678 ? 1 : 0;
    }
    return 0;
}
