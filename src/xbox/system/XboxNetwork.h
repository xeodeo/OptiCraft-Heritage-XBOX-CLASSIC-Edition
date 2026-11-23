#pragma once

#include <stdint.h>

// Shared XNet bring-up and plain TCP calls for everything that talks to the
// network: the UDP debug log (XboxNetLog.cpp) and multiplayer sockets
// (src/java/JavaNetworkXbox.cpp).
//
// Plain C types only: XboxNetwork.cpp is compiled against the XDK headers,
// whose 2003 STL (IStream, String, ...) is not layout-compatible with the
// VS2022 STL the game uses, so no std:: object may cross this boundary.
namespace XboxNetwork
{
// Why the last host name lookup failed (or used the public-DNS fallback); "" if fine.
const char *lastResolveError();
// XNetStartup (security bypassed: the only mode that may reach PCs and
// ordinary servers) + WSAStartup, then waits for the dashboard/DHCP address.
// Safe to call from any thread and more than once.
bool initialize();

// host is a dotted IPv4 address or a DNS name (resolved with XNetDnsLookup).
// Writes the address in network byte order.
bool resolveIPv4(const char* host, unsigned long* address);
bool resolveIPv4Timeout(const char* host, unsigned long* address, int timeoutMs);
// Minecraft SRV record (_minecraft._tcp.<host>) through a public resolver;
// false when the host has none (or is an IP literal).
bool resolveMinecraftSrv(const char* host, char* target, int targetLen, int* port);

const intptr_t kInvalidSocket = -1;

// Blocking-style TCP over a non-blocking socket, so one thread may send while
// another waits in tcpRecv. tcpConnect returns kInvalidSocket on failure.
intptr_t tcpConnect(const char* host, int port);
intptr_t tcpConnectTimeout(const char* host, int port, int timeoutMs);
int tcpRecv(intptr_t socket, char* buffer, int length);          // > 0 bytes, <= 0 closed/error
int tcpRecvTimeout(intptr_t socket, char* buffer, int length, int timeoutMs);
int tcpSend(intptr_t socket, const char* buffer, int length);    // bytes sent, <= 0 error
int tcpSendTimeout(intptr_t socket, const char* buffer, int length, int timeoutMs);
void tcpShutdown(intptr_t socket, bool receiveOnly);
void tcpClose(intptr_t socket);
int lastError();
unsigned int tickMs();
bool hasLink();
}
