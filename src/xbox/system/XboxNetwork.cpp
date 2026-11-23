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

const char *s_lastResolveError = "";

// Minimal DNS A query over UDP to a public resolver. Used when the console's own
// DNS (dashboard settings) fails, e.g. a static IP configured without DNS: the
// FTP and the UDP log work by IP, but every host name lookup fails.
bool queryPublicDns(const char *host, unsigned long resolverIp, unsigned long *address, int timeoutMs)
{
    unsigned char pkt[300];
    int n = 0;
    const unsigned short id = static_cast<unsigned short>(GetTickCount() & 0xFFFF);
    pkt[n++] = static_cast<unsigned char>(id >> 8); pkt[n++] = static_cast<unsigned char>(id & 0xFF);
    pkt[n++] = 0x01; pkt[n++] = 0x00;            // recursion desired
    pkt[n++] = 0; pkt[n++] = 1;                  // one question
    for (int i = 0; i < 6; ++i) pkt[n++] = 0;    // no answers/authority/additional
    const char *label = host;
    while (*label != '\0')
    {
        const char *dot = strchr(label, '.');
        const int len = dot != nullptr ? static_cast<int>(dot - label) : static_cast<int>(strlen(label));
        if (len <= 0 || len > 63 || n + len + 6 >= static_cast<int>(sizeof(pkt)))
            return false;
        pkt[n++] = static_cast<unsigned char>(len);
        memcpy(pkt + n, label, len);
        n += len;
        label += len;
        if (*label == '.')
            ++label;
    }
    pkt[n++] = 0;
    pkt[n++] = 0; pkt[n++] = 1;                  // type A
    pkt[n++] = 0; pkt[n++] = 1;                  // class IN

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
        return false;
    sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = htons(53);
    to.sin_addr.s_addr = resolverIp;
    bool ok = false;
    if (sendto(s, reinterpret_cast<const char *>(pkt), n, 0, reinterpret_cast<sockaddr *>(&to), sizeof(to)) == n)
    {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(s, &rd);
        timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        unsigned char resp[512];
        if (select(0, &rd, nullptr, nullptr, &tv) > 0)
        {
            const int got = recv(s, reinterpret_cast<char *>(resp), sizeof(resp), 0);
            if (got > 12 && resp[0] == pkt[0] && resp[1] == pkt[1] && (resp[3] & 0x0F) == 0)
            {
                const int answers = (resp[6] << 8) | resp[7];
                int p = 12;
                while (p < got && resp[p] != 0)          // skip the question name
                    p += (resp[p] & 0xC0) == 0xC0 ? 1 : resp[p] + 1;
                p += 5;                                  // terminator + type + class
                for (int a = 0; a < answers && p + 10 <= got && !ok; ++a)
                {
                    if ((resp[p] & 0xC0) == 0xC0)
                        p += 2;
                    else
                    {
                        while (p < got && resp[p] != 0)
                            p += resp[p] + 1;
                        ++p;
                    }
                    if (p + 10 > got)
                        break;
                    const int type = (resp[p] << 8) | resp[p + 1];
                    const int rdlen = (resp[p + 8] << 8) | resp[p + 9];
                    p += 10;
                    if (type == 1 && rdlen == 4 && p + 4 <= got)
                    {
                        memcpy(address, resp + p, 4);
                        ok = true;
                    }
                    p += rdlen;
                }
            }
        }
    }
    closesocket(s);
    return ok;
}

// DNS name at resp[p] (labels and compression pointers) -> dotted text.
bool readDnsName(const unsigned char *resp, int got, int p, char *out, int outLen)
{
    int n = 0;
    int jumps = 0;
    while (p < got)
    {
        const int len = resp[p];
        if (len == 0)
        {
            if (n > 0)
                --n; // drop the trailing dot
            out[n] = '\0';
            return n > 0;
        }
        if ((len & 0xC0) == 0xC0)
        {
            if (p + 1 >= got || ++jumps > 16)
                return false;
            p = ((len & 0x3F) << 8) | resp[p + 1];
            continue;
        }
        if (p + 1 + len > got || n + len + 2 > outLen)
            return false;
        memcpy(out + n, resp + p + 1, len);
        n += len;
        out[n++] = '.';
        p += 1 + len;
    }
    return false;
}

// SRV query (_minecraft._tcp.<host>) to a public resolver.
bool queryPublicSrv(const char *name, unsigned long resolverIp, char *target, int targetLen, int *port, int timeoutMs)
{
    unsigned char pkt[300];
    int n = 0;
    const unsigned short id = static_cast<unsigned short>((GetTickCount() * 7) & 0xFFFF);
    pkt[n++] = static_cast<unsigned char>(id >> 8); pkt[n++] = static_cast<unsigned char>(id & 0xFF);
    pkt[n++] = 0x01; pkt[n++] = 0x00;
    pkt[n++] = 0; pkt[n++] = 1;
    for (int i = 0; i < 6; ++i) pkt[n++] = 0;
    const char *label = name;
    while (*label != '\0')
    {
        const char *dot = strchr(label, '.');
        const int len = dot != nullptr ? static_cast<int>(dot - label) : static_cast<int>(strlen(label));
        if (len <= 0 || len > 63 || n + len + 6 >= static_cast<int>(sizeof(pkt)))
            return false;
        pkt[n++] = static_cast<unsigned char>(len);
        memcpy(pkt + n, label, len);
        n += len;
        label += len;
        if (*label == '.')
            ++label;
    }
    pkt[n++] = 0;
    pkt[n++] = 0; pkt[n++] = 33;                 // type SRV
    pkt[n++] = 0; pkt[n++] = 1;

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
        return false;
    sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = htons(53);
    to.sin_addr.s_addr = resolverIp;
    bool ok = false;
    if (sendto(s, reinterpret_cast<const char *>(pkt), n, 0, reinterpret_cast<sockaddr *>(&to), sizeof(to)) == n)
    {
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(s, &rd);
        timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        unsigned char resp[512];
        if (select(0, &rd, nullptr, nullptr, &tv) > 0)
        {
            const int got = recv(s, reinterpret_cast<char *>(resp), sizeof(resp), 0);
            if (got > 12 && resp[0] == pkt[0] && resp[1] == pkt[1] && (resp[3] & 0x0F) == 0)
            {
                const int answers = (resp[6] << 8) | resp[7];
                int p = 12;
                while (p < got && resp[p] != 0)
                    p += (resp[p] & 0xC0) == 0xC0 ? 1 : resp[p] + 1;
                p += 5;
                for (int a = 0; a < answers && p + 10 <= got && !ok; ++a)
                {
                    if ((resp[p] & 0xC0) == 0xC0)
                        p += 2;
                    else
                    {
                        while (p < got && resp[p] != 0)
                            p += resp[p] + 1;
                        ++p;
                    }
                    if (p + 10 > got)
                        break;
                    const int type = (resp[p] << 8) | resp[p + 1];
                    const int rdlen = (resp[p + 8] << 8) | resp[p + 9];
                    p += 10;
                    if (type == 33 && rdlen > 6 && p + rdlen <= got)
                    {
                        *port = (resp[p + 4] << 8) | resp[p + 5];
                        ok = readDnsName(resp, got, p + 6, target, targetLen) && *port > 0;
                    }
                    p += rdlen;
                }
            }
        }
    }
    closesocket(s);
    return ok;
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
            // Wait for the address (DHCP included), but bail quickly if there is no Ethernet link.
            XNADDR addr;
            const int maxSteps = (XNetGetEthernetLinkStatus() == 0) ? 10 : 80;
            for (int i = 0; i < maxSteps && XNetGetTitleXnAddr(&addr) == XNET_GET_XNADDR_PENDING; ++i)
                Sleep(50);
            s_state = State::Ready;
        }
    }
    const bool ready = s_state == State::Ready;
    unlock();
    return ready;
}

bool hasLink()
{
    if (!initialize())
        return false;
    if (XNetGetEthernetLinkStatus() == 0)
        return false;
    XNADDR addr;
    const DWORD st = XNetGetTitleXnAddr(&addr);
    return st != XNET_GET_XNADDR_NONE && st != XNET_GET_XNADDR_PENDING;
}

unsigned int tickMs()
{
    return static_cast<unsigned int>(GetTickCount());
}

bool resolveIPv4Timeout(const char* host, unsigned long* address, int timeoutMs)
{
    if (address == nullptr || host == nullptr || host[0] == '\0' || !initialize())
        return false;

    const unsigned long literal = inet_addr(host);
    if (literal != INADDR_NONE)
    {
        *address = literal;
        return true;
    }

    if (XNetGetEthernetLinkStatus() == 0)
        return false;

    if (timeoutMs <= 0)
        timeoutMs = 1000;

    WSAEVENT event = WSACreateEvent();
    if (event == WSA_INVALID_EVENT)
        return false;
    XNDNS* dns = nullptr;
    bool ok = false;
    if (XNetDnsLookup(host, event, &dns) == 0 && dns != nullptr)
    {
        const DWORD start = GetTickCount();
        while (dns->iStatus == WSAEINPROGRESS)
        {
            const DWORD elapsed = GetTickCount() - start;
            if (elapsed >= static_cast<DWORD>(timeoutMs))
                break;
            DWORD waitSlice = static_cast<DWORD>(timeoutMs) - elapsed;
            if (waitSlice > 50)
                waitSlice = 50;
            WaitForSingleObject(event, waitSlice);
        }
        if (dns->iStatus == 0 && dns->cina > 0)
        {
            *address = dns->aina[0].s_addr;
            ok = true;
        }
        XNetDnsRelease(dns);
    }
    WSACloseEvent(event);
    if (ok)
    {
        s_lastResolveError = "";
        return true;
    }

    // The console DNS failed (or has none configured): ask public resolvers.
    const unsigned long resolvers[] = { inet_addr("8.8.8.8"), inet_addr("1.1.1.1") };
    for (unsigned long r : resolvers)
    {
        if (queryPublicDns(host, r, address, 2000))
        {
            s_lastResolveError = "dns de la consola fallo; resuelto con DNS publico";
            return true;
        }
    }
    s_lastResolveError = "sin DNS: ni el de la consola ni 8.8.8.8/1.1.1.1 respondieron";
    return false;
}

bool resolveMinecraftSrv(const char *host, char *target, int targetLen, int *port)
{
    if (host == nullptr || target == nullptr || port == nullptr || targetLen < 8 || !initialize())
        return false;
    if (inet_addr(host) != INADDR_NONE || strchr(host, '.') == nullptr)
        return false;
    char name[300];
    const int len = static_cast<int>(strlen(host));
    if (len > 250)
        return false;
    memcpy(name, "_minecraft._tcp.", 16);
    memcpy(name + 16, host, len + 1);
    const unsigned long resolvers[] = { inet_addr("8.8.8.8"), inet_addr("1.1.1.1") };
    for (unsigned long r : resolvers)
        if (queryPublicSrv(name, r, target, targetLen, port, 1500))
            return true;
    return false;
}

const char *lastResolveError()
{
    return s_lastResolveError;
}

bool resolveIPv4(const char* host, unsigned long* address)
{
    return resolveIPv4Timeout(host, address, 10000);
}

namespace
{
const long kConnectTimeoutSeconds = 4;
}

intptr_t tcpConnectTimeout(const char* host, int port, int timeoutMs)
{
    if (timeoutMs <= 0)
        timeoutMs = 4000;
    const DWORD start = GetTickCount();
    int dnsBudget = timeoutMs / 2;
    if (dnsBudget < 1500)
        dnsBudget = timeoutMs;
    unsigned long address = 0;
    if (port < 1 || port > 65535 || !resolveIPv4Timeout(host, &address, dnsBudget))
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

    // The game reads and writes the socket from two threads. XNet refuses a
    // call on a socket while another thread is blocked in one
    // (WSAEINPROGRESS), so the socket is non-blocking and tcpRecv/tcpSend
    // wait by polling. Connecting non-blocking also bounds the wait for a
    // server that does not answer (the same fix the Wii port got upstream).
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    if (connect(s, reinterpret_cast<const sockaddr*>(&target), sizeof(target)) != 0)
    {
        const int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
        {
            closesocket(s);
            return kInvalidSocket;
        }
        const DWORD elapsed = GetTickCount() - start;
        int remMs = timeoutMs - static_cast<int>(elapsed);
        if (remMs < 500)
            remMs = 500;
        fd_set writable;
        fd_set failed;
        FD_ZERO(&writable);
        FD_ZERO(&failed);
        FD_SET(s, &writable);
        FD_SET(s, &failed);
        timeval timeout;
        timeout.tv_sec = remMs / 1000;
        timeout.tv_usec = (remMs % 1000) * 1000;
        if (select(0, nullptr, &writable, &failed, &timeout) <= 0 || FD_ISSET(s, &failed) ||
            !FD_ISSET(s, &writable))
        {
            closesocket(s);
            return kInvalidSocket;
        }
    }
    return static_cast<intptr_t>(s);
}

intptr_t tcpConnect(const char* host, int port)
{
    return tcpConnectTimeout(host, port, kConnectTimeoutSeconds * 1000 + 10000);
}

namespace
{
// A 1.2.5 server sends a keep-alive every few seconds, so a socket silent (or
// unable to take data) for this long is dead; give up instead of polling on.
const DWORD kStallTimeoutMs = 45000;

bool shouldRetry()
{
    const int error = WSAGetLastError();
    return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
}
} // namespace

int tcpRecvTimeout(intptr_t s, char* buffer, int length, int timeoutMs)
{
    if (timeoutMs <= 0)
        timeoutMs = 1000;
    const DWORD start = GetTickCount();
    for (;;)
    {
        const int count = recv(static_cast<SOCKET>(s), buffer, length, 0);
        if (count >= 0 || !shouldRetry())
            return count;
        if (GetTickCount() - start > static_cast<DWORD>(timeoutMs))
        {
            WSASetLastError(WSAETIMEDOUT);
            return -1;
        }
        Sleep(2);
    }
}

int tcpRecv(intptr_t s, char* buffer, int length)
{
    return tcpRecvTimeout(s, buffer, length, static_cast<int>(kStallTimeoutMs));
}

int tcpSendTimeout(intptr_t s, const char* buffer, int length, int timeoutMs)
{
    if (timeoutMs <= 0)
        timeoutMs = 1000;
    const DWORD start = GetTickCount();
    for (;;)
    {
        const int count = send(static_cast<SOCKET>(s), buffer, length, 0);
        if (count >= 0 || !shouldRetry())
            return count;
        if (GetTickCount() - start > static_cast<DWORD>(timeoutMs))
        {
            WSASetLastError(WSAETIMEDOUT);
            return -1;
        }
        Sleep(1);
    }
}

int tcpSend(intptr_t s, const char* buffer, int length)
{
    return tcpSendTimeout(s, buffer, length, static_cast<int>(kStallTimeoutMs));
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
bool resolveMinecraftSrv(const char*, char*, int, int*) { return false; }
bool resolveIPv4Timeout(const char*, unsigned long*, int) { return false; }
intptr_t tcpConnect(const char*, int) { return kInvalidSocket; }
intptr_t tcpConnectTimeout(const char*, int, int) { return kInvalidSocket; }
int tcpRecv(intptr_t, char*, int) { return -1; }
int tcpRecvTimeout(intptr_t, char*, int, int) { return -1; }
int tcpSend(intptr_t, const char*, int) { return -1; }
int tcpSendTimeout(intptr_t, const char*, int, int) { return -1; }
void tcpShutdown(intptr_t, bool) {}
void tcpClose(intptr_t) {}
int lastError() { return 0; }
unsigned int tickMs() { return 0; }
bool hasLink() { return false; }
}

#endif // XBOX_NETLOG_HOST || !NO_NETWORK

#endif // XBOX_PLATFORM
