// XboxNetLog.cpp — debug: mirror every log line over UDP to a PC.
//
// On the console the log otherwise only reaches T:\debug.log, which can be
// read over FTP only after powering off (launching the title unloads the
// dashboard and its FTP server). With XBOX_NETLOG_HOST set at configure time
// each line is also sent as one UDP datagram to <host>:9999, where
// xbox-spike/escuchar_log.py prints it live — up to the last line before a
// hang. Uses the devkit XNet library with security bypassed (the only mode
// that may talk to a PC); the Xbox takes its IP from the dashboard settings.
#ifdef XBOX_PLATFORM

#include "xbox/XboxXtl.h"

#include <cstring>

#ifdef XBOX_NETLOG_HOST

namespace
{
const unsigned short kPort = 9999;
SOCKET s_socket = INVALID_SOCKET;
sockaddr_in s_target = {};
}

extern "C" void xboxNetLogInit()
{
    XNetStartupParams params;
    std::memset(&params, 0, sizeof(params));
    params.cfgSizeOfStruct = sizeof(params);
    params.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    if (XNetStartup(&params) != 0)
        return;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return;

    // Wait (up to 6 s) for the address, DHCP included; lines sent before the
    // stack has one are dropped.
    XNADDR addr;
    for (int i = 0; i < 120 && XNetGetTitleXnAddr(&addr) == XNET_GET_XNADDR_PENDING; ++i)
        Sleep(50);

    s_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_socket == INVALID_SOCKET)
        return;
    s_target.sin_family = AF_INET;
    s_target.sin_port = htons(kPort);
    s_target.sin_addr.s_addr = inet_addr(XBOX_NETLOG_HOST);
}

extern "C" void xboxNetLogSend(const char* line)
{
    if (s_socket == INVALID_SOCKET || !line)
        return;
    sendto(s_socket, line, static_cast<int>(std::strlen(line)), 0,
           reinterpret_cast<const sockaddr*>(&s_target), sizeof(s_target));
}

#else

extern "C" void xboxNetLogInit() {}
extern "C" void xboxNetLogSend(const char*) {}

#endif // XBOX_NETLOG_HOST

#endif // XBOX_PLATFORM
