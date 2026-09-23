#ifdef XBOX_PLATFORM
#include "platform/Log.h"
#include "xbox/system/XboxBootstrap.h"
#include "lwjgl/Display.h"

namespace XboxBootstrap
{

bool initialize()
{
	MC_LOG_INFO("xbox", "main() entered\n");
	MC_LOG_INFO("xbox", "initializing display...\n");
	lwjgl::Display::create();
	MC_LOG_INFO("xbox", "display ready\n");
	return true;
}

void shutdown()
{
	MC_LOG_INFO("xbox", "shutdown()\n");
}

} // namespace XboxBootstrap
#endif
