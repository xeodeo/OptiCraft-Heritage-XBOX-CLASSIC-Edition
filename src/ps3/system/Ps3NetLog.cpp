// Ps3NetLog.cpp -- where log lines go on the PS3 (McLog's platform sink).
//
// Every line goes to the lv2 TTY (stdout), which RPCS3 shows in its log and a
// debug console would show in its TTY window. With PS3_NETLOG_HOST set at
// configure time each line is also sent as one UDP datagram to <host>:9999,
// the same port the Xbox build uses, so xbox-spike/escuchar_log.py prints the
// console's log live -- up to the last line before a hang. The console takes
// its address from the XMB network settings.
#ifdef PS3_PLATFORM

#include <cstdio>
#include <cstring>

#ifdef PS3_NETLOG_HOST
#include <arpa/inet.h>
#include <net/net.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
const unsigned short kPort = 9999;
int s_socket = -1;
sockaddr_in s_target = {};
bool s_tried = false;

void openSocket()
{
    s_tried = true;
    if (netInitialize() < 0)
        return;
    s_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_socket < 0)
        return;
    s_target.sin_family = AF_INET;
    s_target.sin_port = htons(kPort);
    s_target.sin_addr.s_addr = inet_addr(PS3_NETLOG_HOST);
}
}

extern "C" void ps3NetLogInit()
{
    if (!s_tried)
        openSocket();
}

extern "C" void ps3PlatformLogWrite(const char* line)
{
    if (line == nullptr)
        return;
    std::fputs(line, stdout);
    if (!s_tried)
        openSocket();
    if (s_socket >= 0)
        sendto(s_socket, line, std::strlen(line), 0, reinterpret_cast<const sockaddr*>(&s_target),
               sizeof(s_target));
}

#else

extern "C" void ps3NetLogInit() {}

extern "C" void ps3PlatformLogWrite(const char* line)
{
    if (line != nullptr)
        std::fputs(line, stdout);
}

#endif // PS3_NETLOG_HOST

#endif // PS3_PLATFORM
