#ifdef XBOX_PLATFORM
#include "xbox/system/XboxNetwork.h"

#if defined(XBOX_NETLOG_HOST) || !defined(NO_NETWORK)

#include "xbox/XboxXtl.h"

#include <string.h>

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
        memset(&params, 0, sizeof(params));
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

bool resolveIPv4(const char* host, unsigned long* address)
{
    if (address == nullptr || host == nullptr || host[0] == '\0' || !initialize())
        return false;

    const unsigned long literal = inet_addr(host);
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
    if (XNetDnsLookup(host, event, &dns) == 0 && dns != nullptr)
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

intptr_t tcpConnect(const char* host, int port)
{
    unsigned long address = 0;
    if (port < 1 || port > 65535 || !resolveIPv4(host, &address))
        return kInvalidSocket;

    const SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return kInvalidSocket;

    // Minecraft's protocol is many small packets; don't hold them back.
    BOOL noDelay = TRUE;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

    sockaddr_in target;
    memset(&target, 0, sizeof(target));
    target.sin_family = AF_INET;
    target.sin_port = htons(static_cast<u_short>(port));
    target.sin_addr.s_addr = address;
    if (connect(s, reinterpret_cast<const sockaddr*>(&target), sizeof(target)) != 0)
    {
        closesocket(s);
        return kInvalidSocket;
    }

    // The game reads and writes the socket from two threads. XNet refuses a
    // call on a socket while another thread is blocked in one
    // (WSAEINPROGRESS), so the socket is non-blocking and tcpRecv/tcpSend
    // wait by polling.
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    return static_cast<intptr_t>(s);
}

namespace
{
bool shouldRetry()
{
    const int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
}
} // namespace

int tcpRecv(intptr_t s, char* buffer, int length)
{
    for (;;)
    {
        const int count = recv(static_cast<SOCKET>(s), buffer, length, 0);
        if (count >= 0 || !shouldRetry())
            return count;
        Sleep(2);
    }
}

int tcpSend(intptr_t s, const char* buffer, int length)
{
    for (;;)
    {
        const int count = send(static_cast<SOCKET>(s), buffer, length, 0);
        if (count >= 0 || !shouldRetry())
            return count;
        Sleep(1);
    }
}

void tcpShutdown(intptr_t s, bool receiveOnly)
{
    shutdown(static_cast<SOCKET>(s), receiveOnly ? SD_RECEIVE : SD_BOTH);
}

void tcpClose(intptr_t s)
{
    closesocket(static_cast<SOCKET>(s));
}

int lastError()
{
    return WSAGetLastError();
}
} // namespace XboxNetwork

#else

// Neither the network log nor multiplayer is built in: xnet.lib is not linked.
namespace XboxNetwork
{
bool initialize() { return false; }
bool resolveIPv4(const char*, unsigned long*) { return false; }
intptr_t tcpConnect(const char*, int) { return kInvalidSocket; }
int tcpRecv(intptr_t, char*, int) { return -1; }
int tcpSend(intptr_t, const char*, int) { return -1; }
void tcpShutdown(intptr_t, bool) {}
void tcpClose(intptr_t) {}
int lastError() { return 0; }
}

#endif // XBOX_NETLOG_HOST || !NO_NETWORK

#endif // XBOX_PLATFORM
