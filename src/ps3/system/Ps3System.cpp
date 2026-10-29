#ifdef PS3_PLATFORM

#include "ps3/system/Ps3System.h"

#include "platform/Log.h"

#include <sysutil/sysutil.h>

namespace
{
volatile bool s_exitRequested = false;

void onSystemEvent(u64 status, u64 param, void* userdata)
{
    (void)param;
    (void)userdata;
    switch (status)
    {
        case SYSUTIL_EXIT_GAME:
            MC_LOG_INFO("ps3", "XMB asked the game to quit\n");
            s_exitRequested = true;
            break;
        case SYSUTIL_MENU_OPEN:
            MC_LOG_INFO("ps3", "XMB menu open\n");
            break;
        case SYSUTIL_MENU_CLOSE:
            MC_LOG_INFO("ps3", "XMB menu closed\n");
            break;
        default:
            break;
    }
}
}

namespace Ps3System
{
void initialize()
{
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, onSystemEvent, nullptr);
}

void pumpEvents()
{
    sysUtilCheckCallback();
}

bool exitRequested()
{
    return s_exitRequested;
}
}

#endif // PS3_PLATFORM
