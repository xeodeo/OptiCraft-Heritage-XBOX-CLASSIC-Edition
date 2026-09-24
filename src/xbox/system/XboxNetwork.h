#pragma once

#include <string>

// Shared XNet bring-up for everything that talks to the network: the UDP
// debug log (XboxNetLog.cpp) and multiplayer sockets (JavaNetwork_xbox.cpp).
namespace XboxNetwork
{
// XNetStartup (security bypassed: the only mode that may reach PCs and
// ordinary servers) + WSAStartup, then waits for the dashboard/DHCP address.
// Safe to call from any thread and more than once; returns false when the
// stack could not start.
bool initialize();

// host is a dotted IPv4 address or a DNS name (resolved with XNetDnsLookup).
// Writes the address in network byte order.
bool resolveIPv4(const std::string& host, unsigned long* address);
}
