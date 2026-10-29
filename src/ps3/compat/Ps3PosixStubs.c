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


#ifdef PS3_PLATFORM
#include <ppu-lv2.h>
#include <sys/systime.h>
#include <sys/time.h>

// newlib's gettimeofday goes through PSL1GHT's _gettimeofday_r hook, which never
// fills the time in: std::chrono::steady_clock/system_clock (libstdc++ builds
// both on gettimeofday here) stood still, and timed loops such as the startup
// logos never ended. Read the lv2 clock directly (syscall 145).
int gettimeofday(struct timeval* tv, void* tz)
{
    (void)tz;
    if (tv == NULL)
        return 0;
    u64 sec = 0;
    u64 nsec = 0;
    sysGetCurrentTime(&sec, &nsec);
    tv->tv_sec = (time_t)sec;
    tv->tv_usec = (suseconds_t)(nsec / 1000);
    return 0;
}
#endif // PS3_PLATFORM
