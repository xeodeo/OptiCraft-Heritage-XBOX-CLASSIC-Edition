#ifdef XBOX_PLATFORM
#include "xbox/system/XboxNetwork.h"

#if defined(XBOX_NETLOG_HOST) || !defined(NO_NETWORK)

#include "xbox/XboxXtl.h"

#include <cstring>

namespace
{
enum class State
{
    NotStarted,
    Ready,
    Failed
};

volatile LONG s_initLock = 0;
State s_state = State::NotStarted;

void lock()
{
    while (InterlockedCompareExchange(&s_initLock, 1, 0) != 0)
        Sleep(1);
}

void unlock()
{
    InterlockedExchange(&s_initLock, 0);
}
} // namespace

namespace XboxNetwork
{
bool initialize()
{
    lock();
    if (s_state == State::NotStarted)
    {
        s_state = State::Failed;
        XNetStartupParams params;
        std::memset(&params, 0, sizeof(params));
        params.cfgSizeOfStruct = sizeof(params);
        params.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
        WSADATA wsa;
        if (XNetStartup(&params) == 0 && WSAStartup(MAKEWORD(2, 2), &wsa) == 0)
        {
            // Wait (up to 6 s) for the address, DHCP included.
            XNADDR addr;
            for (int i = 0; i < 120 && XNetGetTitleXnAddr(&addr) == XNET_GET_XNADDR_PENDING; ++i)
                Sleep(50);
            s_state = State::Ready;
        }
    }
    const bool ready = s_state == State::Ready;
    unlock();
    return ready;
}

bool resolveIPv4(const std::string& host, unsigned long* address)
{
    if (address == nullptr || host.empty() || !initialize())
        return false;

    const unsigned long literal = inet_addr(host.c_str());
    if (literal != INADDR_NONE)
    {
        *address = literal;
        return true;
    }

    WSAEVENT event = WSACreateEvent();
    if (event == WSA_INVALID_EVENT)
        return false;
    XNDNS* dns = nullptr;
    bool ok = false;
    if (XNetDnsLookup(host.c_str(), event, &dns) == 0 && dns != nullptr)
    {
        // Resolution runs in the stack; give it up to 10 s.
        for (int i = 0; i < 200 && dns->iStatus == WSAEINPROGRESS; ++i)
            WaitForSingleObject(event, 50);
        if (dns->iStatus == 0 && dns->cina > 0)
        {
            *address = dns->aina[0].s_addr;
            ok = true;
        }
        XNetDnsRelease(dns);
    }
    WSACloseEvent(event);
    return ok;
}
} // namespace XboxNetwork

#else

// Neither the network log nor multiplayer is built in: xnet.lib is not linked.
namespace XboxNetwork
{
bool initialize() { return false; }
bool resolveIPv4(const std::string&, unsigned long*) { return false; }
}

#endif // XBOX_NETLOG_HOST || !NO_NETWORK

#endif // XBOX_PLATFORM
