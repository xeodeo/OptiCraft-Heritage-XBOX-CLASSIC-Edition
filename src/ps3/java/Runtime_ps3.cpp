// Runtime_ps3.cpp -- PS3 implementation of java/Runtime.h.
//
// Game processes get ~213 MB of the 256 MB XDR main memory (the rest is the
// system); RSX local memory is separate and not counted here. lv2 reports both
// numbers through sys_memory_get_user_memory_size (syscall 352), which PSL1GHT
// does not wrap.
#ifdef PS3_PLATFORM

#include "java/Runtime.h"
#include "ps3/system/Ps3Memory.h"

Runtime Runtime::instance;

Runtime &Runtime::getRuntime()
{
	return instance;
}

long_t Runtime::maxMemory()
{
	const Ps3Memory::Info info = Ps3Memory::query();
	return info.totalBytes > 0 ? static_cast<long_t>(info.totalBytes) : 1;
}

long_t Runtime::totalMemory()
{
	return static_cast<long_t>(Ps3Memory::query().totalBytes);
}

long_t Runtime::freeMemory()
{
	return static_cast<long_t>(Ps3Memory::query().availableBytes);
}

#endif // PS3_PLATFORM
