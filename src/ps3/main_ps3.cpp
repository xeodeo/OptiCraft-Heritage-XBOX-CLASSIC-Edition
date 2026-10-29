// PS3 entry point. Same flow as main_xbox.cpp / main_wii.cpp: log first, then the
// display, then Minecraft::start(). Launched from the XMB as
// /dev_hdd0/game/<title id>/USRDIR/EBOOT.BIN (an NPDRM SELF; see cmake/ps3.cmake).
#ifdef PS3_PLATFORM

#include "platform/Log.h"
#include "client/Minecraft.h"
#include "java/String.h"
#include "lwjgl/Display.h"
#include "ps3/system/Ps3Memory.h"
#include "ps3/system/Ps3Paths.h"
#include "ps3/system/Ps3System.h"

#include <sys/process.h>
#include <sys/thread.h>

// Priority 1001 (normal game priority), 1 MB primary thread stack: the maximum
// lv2 grants the main thread. The world generator and the renderer recurse.
SYS_PROCESS_PARAM(1001, 0x100000);

extern "C" void ps3NetLogInit();

namespace
{
// Quitting froze in the first throw of the session (LoadingScreenRenderer's
// MinecraftError). Throw one at boot on a side thread and log whether it is
// caught, so a broken unwinder shows up in the log instead of at quit.
void exceptionProbe(void*)
{
	MC_LOG_INFO("ps3", "exception probe: throwing\n");
	try
	{
		throw 42;
	}
	catch (int value)
	{
		MC_LOG_INFO("ps3", "exception probe: caught %d\n", value);
	}
	sysThreadExit(0);
}
}

int main(int argc, char** argv)
{
	// Started first so a boot that dies anywhere after this names its last step.
	ps3NetLogInit();  // no-op unless built with PS3_NETLOG_HOST
	Ps3Paths::initialize(argc > 0 ? argv[0] : nullptr);
	const Ps3Memory::Info memory = Ps3Memory::query();
	MC_LOG_INFO("ps3", "BUILD %s %s, install %s, memory %u KB (%u KB free)\n", __DATE__, __TIME__,
	            Ps3Paths::installDir(), memory.totalBytes / 1024, memory.availableBytes / 1024);

	Ps3System::initialize();
	sys_ppu_thread_t probe;
	sysThreadCreate(&probe, exceptionProbe, nullptr, 1500, 64 * 1024, 0, const_cast<char*>("exception probe"));
	MC_LOG_INFO("ps3", "initializing display...\n");
	lwjgl::Display::create();
	MC_LOG_INFO("ps3", "display ready, handing off to Minecraft::start()\n");

	jstring username = "Player";
	jstring auth = "-";
	Minecraft::start(&username, &auth);
	MC_LOG_INFO("ps3", "Minecraft::start returned; back to the XMB\n");
	return 0;
}

#endif // PS3_PLATFORM
