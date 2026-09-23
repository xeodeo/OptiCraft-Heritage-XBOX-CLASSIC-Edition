// Xbox entry point. Mirrors Wii/PS2 main() flow adapted to Xbox runtime (void __cdecl main()).
#ifdef XBOX_PLATFORM

#include "platform/Log.h"
#include "client/Minecraft.h"
#include "java/String.h"
#include "xbox/system/XboxBootstrap.h"
#include "xbox/system/XboxWritableRoot.h"

#include <cstdio>
#include <float.h>
#include <intrin.h>
#include <math.h>
#include <xmmintrin.h>

extern "C" int __isa_available;  // vcruntime CPU dispatch level

namespace XboxSelfTest { void run(); }
extern "C" void xboxNetLogInit();

namespace
{
// The Xbox CPU is a Pentium III: SSE, no SSE2. The VS2022 CRT picks SSE2
// implementations of memmove/memset and of the libm functions at runtime from
// CPUID, and an emulator can report more than the real chip has. Pin the CRT
// to what the hardware actually supports so emulator and console run the same
// (x87) code paths, and put MXCSR in its documented default (all SSE
// exceptions masked, round to nearest).
void pinCpuFeaturesToPentiumIII()
{
	int regs[4] = {};
	__cpuid(regs, 1);
	const bool cpuidSse2 = (regs[3] & (1 << 26)) != 0;
	const unsigned int mxcsrBefore = _mm_getcsr();
	const int isaBefore = __isa_available;
	const int sse2MathAfter = _set_SSE2_enable(0);
	if (__isa_available > 0)
		__isa_available = 0;  // __ISA_AVAILABLE_X86
	_mm_setcsr(0x1F80);

	MC_LOG_INFO("xbox", "cpu: cpuid_sse2=%d isa=%d->%d sse2_math=%d mxcsr=%04x->%04x\n",
	            cpuidSse2 ? 1 : 0, isaBefore, __isa_available, sse2MathAfter,
	            mxcsrBefore, _mm_getcsr());
}
} // namespace

void __cdecl main()
{
	// On the console the in-memory log cannot be read; write it to the title
	// data drive as well (E:\TDATA\<title id>\debug.log, reachable over FTP).
	// Opened first so a boot that dies anywhere after this names its last step.
	xboxNetLogInit();  // no-op unless built with XBOX_NETLOG_HOST
	const char* writableRoot = XboxWritableRoot::get();
	char logDir[8];
	std::snprintf(logDir, sizeof(logDir), "%s\\", writableRoot);  // "T:\", not "T:"
	const bool logFile = McLog::openSessionFile(logDir);
	MC_LOG_INFO("xbox", "writable root %s, debug.log %s, build %s %s\n", writableRoot,
	            logFile ? "open" : "FAILED", __DATE__, __TIME__);
	pinCpuFeaturesToPentiumIII();
	XboxSelfTest::run();

	if (!XboxBootstrap::initialize())
		return;

	MC_LOG_INFO("xbox", "handing off to Minecraft::start()\n");
	jstring username = "Player";
	jstring auth = "-";
	Minecraft::start(&username, &auth);
	MC_LOG_INFO("xbox", "Minecraft::start returned\n");

	XboxBootstrap::shutdown();
}

#endif
