// POSIX functions libstdc++ references that PSL1GHT's newlib 1.20 does not have.
// They are only reached through std::filesystem paths the game does not use.
#ifdef PS3_PLATFORM

#include <errno.h>

long pathconf(const char* path, int name)
{
    (void)path;
    (void)name;
    errno = ENOSYS;
    return -1;
}

#endif // PS3_PLATFORM

