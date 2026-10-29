#ifdef PS3_PLATFORM

#include "ps3/system/Ps3Paths.h"

#include <cstring>
#include <string>

namespace
{
#ifndef PS3_TITLE_ID
#define PS3_TITLE_ID "OPTC00001"
#endif

std::string g_installDir = "/dev_hdd0/game/" PS3_TITLE_ID "/USRDIR";
}

namespace Ps3Paths
{
void initialize(const char* argv0)
{
    // argv[0] is the full path of EBOOT.BIN when launched from the XMB, e.g.
    // "/dev_hdd0/game/OPTC00001/USRDIR/EBOOT.BIN". RPCS3 passes the same shape.
    if (argv0 == nullptr || argv0[0] != '/')
        return;
    std::string path(argv0);
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos || slash == 0)
        return;
    g_installDir = path.substr(0, slash);
}

const char* installDir() { return g_installDir.c_str(); }
const char* writableDir() { return g_installDir.c_str(); }
}

#endif // PS3_PLATFORM
