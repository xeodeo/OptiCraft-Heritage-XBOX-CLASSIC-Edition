// Runtime_xbox.cpp — Xbox implementation of java/Runtime.h.
//
// The Xbox has one unified 64 MB pool (128 MB on dev kits, capped to 64 MB by
// imagebld /LIMITMEM). XAPI's GlobalMemoryStatus reports it directly.
#ifdef XBOX_PLATFORM

#include "xbox/XboxXtl.h"

#include "java/Runtime.h"

Runtime Runtime::instance;

Runtime &Runtime::getRuntime()
{
	return instance;
}

namespace
{
MEMORYSTATUS queryMemory()
{
	MEMORYSTATUS status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatus(&status);
	return status;
}
} // namespace

long_t Runtime::maxMemory()
{
	const MEMORYSTATUS status = queryMemory();
	return status.dwTotalPhys > 0 ? static_cast<long_t>(status.dwTotalPhys) : 1;
}

long_t Runtime::totalMemory()
{
	// Everything not free is committed to the title (code, heap, D3D, kernel).
	const MEMORYSTATUS status = queryMemory();
	return static_cast<long_t>(status.dwTotalPhys);
}

long_t Runtime::freeMemory()
{
	const MEMORYSTATUS status = queryMemory();
	return static_cast<long_t>(status.dwAvailPhys);
}

#endif // XBOX_PLATFORM
