#include "java/JavaNetwork.h"

#if defined(PS2_PLATFORM) && defined(PS2_ENABLE_NETWORK)

#include "ps2/network/Ps2Network.h"
#include "platform/Log.h"
#include "platform/Mutex.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <delaythread.h>
#include <cerrno>
#include <cstring>
#include <istream>
#include <mutex>
#include <ostream>
#include <streambuf>

#ifdef PS2_REMOTE_DEBUG
// The public ps2ips header exposes only init/deinit, but libps2ips also exports
// these low-level EE-to-IOP socket RPC entry points.
extern "C" {
int ps2ipc_socket(int domain, int type, int protocol);
int ps2ipc_bind(int s, const struct sockaddr *name, int namelen);
int ps2ipc_connect(int s, const struct sockaddr *name, int namelen);
int ps2ipc_recv(int s, void *mem, int len, unsigned int flags);
int ps2ipc_send(int s, const void *data, int len, unsigned int flags);
int ps2ipc_disconnect(int s);
int ps2ipc_ioctl(int s, long cmd, void *argp);
int ps2ipc_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int ps2ipc_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
}
#endif

namespace JavaNetwork
{
namespace
{
#ifdef PS2_REMOTE_DEBUG
constexpr long kIopFionbio = 0x8004667eL; // _IOW('f', 126, unsigned long) on IOP.

// ps2ips copies sockaddr_in verbatim to the IOP. Build dotted IPv4 literals
// with the same byte layout as PS2SDK's IP4_ADDR macro; EE newlib inet_aton()
// does not produce the wire-compatible representation required by this path.
bool parseIopIpv4Literal(const std::string &text, in_addr &address)
{
    std::uint32_t octets[4]{};
    std::size_t cursor = 0;

    for (int index = 0; index < 4; ++index)
    {
        if (cursor >= text.size() || text[cursor] < '0' || text[cursor] > '9')
            return false;

        std::uint32_t value = 0;
        do
        {
            value = value * 10u + static_cast<std::uint32_t>(text[cursor] - '0');
            if (value > 255u)
                return false;
            ++cursor;
        } while (cursor < text.size() && text[cursor] >= '0' && text[cursor] <= '9');

        octets[index] = value;
        if (index < 3)
        {
            if (cursor >= text.size() || text[cursor] != '.')
                return false;
            ++cursor;
        }
    }

    if (cursor != text.size())
        return false;

    address.s_addr = octets[0]
        | (octets[1] << 8)
        | (octets[2] << 16)
        | (octets[3] << 24);
    return true;
}

#else
constexpr int kSocketDontWait = MSG_DONTWAIT;
#endif
class Ps2Socket final : public Socket
{
public:
    ~Ps2Socket() override { releaseSocket(); }

    bool connect(const std::string &host, int port) override
    {
        releaseSocket();
        if (port < 1 || port > 65535 || host.empty())
            return false;

        MC_LOG_INFO("network", "[PS2] socket request %s:%d\n", host.c_str(), port);
        McLog::flush();
        if (!Ps2Network::initialize())
        {
            MC_LOG_WARN("network", "[PS2] network initialization not ready\n");
            return false;
        }

        closing.store(false, std::memory_order_release);
        receivedBytes.store(0, std::memory_order_release);
        sentBytes.store(0, std::memory_order_release);
        remoteAddress = host + ":" + std::to_string(port);

        sockaddr_in target{};
#ifdef PS2_REMOTE_DEBUG
        // ps2ips copies this 16-byte sockaddr verbatim to IOP lwIP.
        target.sin_len = static_cast<unsigned char>(sizeof(target));
#endif
        target.sin_family = AF_INET;
        target.sin_port = htons(static_cast<unsigned short>(port));

#ifdef PS2_REMOTE_DEBUG
        if (!parseIopIpv4Literal(host, target.sin_addr))
        {
            hostent *resolved = gethostbyname(host.c_str());
            if (resolved == nullptr || resolved->h_addr_list == nullptr ||
                resolved->h_addr_list[0] == nullptr)
                return false;
            std::memcpy(&target.sin_addr, resolved->h_addr_list[0], sizeof(target.sin_addr));
        }

        return connectRemote(target, host, port);
#else
        if (inet_aton(host.c_str(), &target.sin_addr) == 0)
        {
            hostent *resolved = gethostbyname(host.c_str());
            if (resolved == nullptr || resolved->h_addr_list == nullptr ||
                resolved->h_addr_list[0] == nullptr)
                return false;
            std::memcpy(&target.sin_addr, resolved->h_addr_list[0], sizeof(target.sin_addr));
        }

        const int newFd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (newFd < 0)
            return false;

        // Player movement and interaction packets are small and latency-sensitive.
        // Disable Nagle so lwIP does not deliberately hold them while waiting for
        // another packet to coalesce. Failure is non-fatal on older SDK builds.
        const int noDelay = 1;
        (void)::setsockopt(newFd, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));

        fd.store(newFd, std::memory_order_release);

        MC_LOG_INFO("network", "[PS2] TCP connect() fd=%d -> %s:%d\n", newFd, host.c_str(), port);
        McLog::flush();
        if (::connect(newFd, reinterpret_cast<sockaddr *>(&target), sizeof(target)) < 0)
        {
            const int errorCode = errno;
            MC_LOG_WARN("network", "[PS2] TCP connect failed errno=%d\n", errorCode);
            releaseSocket();
            return false;
        }
        MC_LOG_INFO("network", "[PS2] TCP connected to %s:%d\n", host.c_str(), port);
        McLog::flush();
        return true;
#endif
    }

    int read(char *buffer, int length) override
    {
        if (buffer == nullptr || length <= 0)
            return -1;

        while (!closing.load(std::memory_order_acquire))
        {
            const int socketFd = fd.load(std::memory_order_acquire);
            if (socketFd < 0)
                return -1;

#ifdef PS2_REMOTE_DEBUG
            const int count = ps2ipc_recv(socketFd, buffer, length, 0);
#else
            const int count = static_cast<int>(::recv(socketFd, buffer,
                                                      static_cast<std::size_t>(length),
                                                      kSocketDontWait));
#endif
            if (count > 0)
            {
                receivedBytes.fetch_add(static_cast<std::size_t>(count), std::memory_order_relaxed);
                return count;
            }
            if (count == 0)
                return 0;
#ifdef PS2_REMOTE_DEBUG
            // ps2ips does not transport IOP errno for recv(). On a nonblocking
            // socket, a negative result with no pending SO_ERROR is the normal
            // would-block case. A pending asynchronous socket error is fatal.
            const int socketError = remoteSocketError(socketFd);
            if (socketError > 0)
            {
                MC_LOG_WARN("network", "[PS2] ps2ips recv failed fd=%d so_error=%d\n",
                            socketFd, socketError);
                closing.store(true, std::memory_order_release);
                return -1;
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                return count;
#endif

            // Keep recv cooperative on real hardware. A blocking recv() otherwise
            // has to be interrupted with shutdown(), which can stall the PS2
            // network stack during disconnect.
            DelayThread(2000);
        }

        return -1;
    }

    bool write(const char *buffer, int length) override
    {
        if (buffer == nullptr)
            return false;
        if (length <= 0)
            return true;

        std::lock_guard<PlatformMutex> guard(writeLock);
        int offset = 0;
        while (offset < length)
        {
            const int socketFd = fd.load(std::memory_order_acquire);
            if (socketFd < 0 || closing.load(std::memory_order_acquire))
                return false;
#ifdef PS2_REMOTE_DEBUG
            const int count = ps2ipc_send(socketFd, buffer + offset, length - offset, 0);
#else
            const int count = static_cast<int>(::send(socketFd, buffer + offset,
                                                      static_cast<std::size_t>(length - offset),
                                                      kSocketDontWait));
#endif
            if (count > 0)
            {
                sentBytes.fetch_add(static_cast<std::size_t>(count), std::memory_order_relaxed);
                offset += count;
                continue;
            }
#ifdef PS2_REMOTE_DEBUG
            if (count < 0)
            {
                const int socketError = remoteSocketError(socketFd);
                if (socketError > 0)
                {
                    MC_LOG_WARN("network", "[PS2] ps2ips send failed fd=%d so_error=%d\n",
                                socketFd, socketError);
                    closing.store(true, std::memory_order_release);
                    return false;
                }
                DelayThread(2000);
                continue;
            }
#else
            if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                DelayThread(2000);
                continue;
            }
#endif
            return false;
        }
        return true;
    }

    bool flush() override
    {
        return fd.load(std::memory_order_acquire) >= 0 &&
               !closing.load(std::memory_order_acquire);
    }

    void interruptRead() override
    {
        // read() polls cooperatively, so marking the socket as closing is enough
        // to wake the reader without a synchronous lwIP shutdown() call.
        closing.store(true, std::memory_order_release);
    }

    void close() override
    {
        // The NetworkManager joins the read/write threads before destroying this
        // socket. Avoid shutdown() here: on real PS2 hardware it can stall the
        // EE/IOP networking path at the exact moment the user disconnects.
        closing.store(true, std::memory_order_release);
    }

    std::string getRemoteSocketAddress() const override { return remoteAddress; }
    std::size_t getReceivedByteCount() const override { return receivedBytes.load(std::memory_order_relaxed); }
    std::size_t getSentByteCount() const override { return sentBytes.load(std::memory_order_relaxed); }

private:
#ifdef PS2_REMOTE_DEBUG
    static int remoteSocketError(int socketFd)
    {
        int socketError = 0;
        socklen_t length = sizeof(socketError);
        return ps2ipc_getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &socketError, &length) < 0
            ? -1
            : socketError;
    }

    bool connectRemote(const sockaddr_in &target, const std::string &host, int port)
    {
        // Keep the IOP descriptor itself. libcglue wraps it in a separate EE fd
        // and collapses negative ps2ips results to ENFILE, losing the socket's
        // actual RPC state and error information.
        const int newFd = ps2ipc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (newFd < 0)
        {
            MC_LOG_WARN("network", "[PS2] ps2ips socket RPC failed\n");
            return false;
        }

        fd.store(newFd, std::memory_order_release);

        // The ps2link-owned PS2IP-NM stack is already configured on sm0. Bind
        // this fresh client PCB to that known local address before connect() so
        // lwIP does not have to infer the source interface from an unbound PCB.
        // This is intentionally remote-debug-only; normal builds keep the
        // application-owned ps2ip stack and its ordinary auto-bind behavior.
        std::uint32_t localAddressNetworkOrder = 0;
        if (!Ps2Network::localAddressNetworkOrder(localAddressNetworkOrder))
        {
            MC_LOG_WARN("network", "[PS2] ps2ips shared local address unavailable; cannot bind fd=%d\n",
                        newFd);
            releaseSocket();
            return false;
        }

        sockaddr_in local{};
        local.sin_len = static_cast<unsigned char>(sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = 0; // Let lwIP choose an ephemeral source port.
        local.sin_addr.s_addr = localAddressNetworkOrder;

        const int bindResult = ps2ipc_bind(
            newFd, reinterpret_cast<const sockaddr *>(&local), sizeof(local));
        if (bindResult < 0)
        {
            MC_LOG_WARN("network", "[PS2] ps2ips bind failed fd=%d result=%d\n",
                        newFd, bindResult);
            releaseSocket();
            return false;
        }

        // The ps2link-owned PS2IP-NM stack requires the handshake while the
        // socket is blocking. Enabling FIONBIO before connect() returns without
        // emitting a SYN on hardware, so switch only established sockets to
        // nonblocking mode for the Java stream implementation.
        const int connectResult = ps2ipc_connect(
            newFd, reinterpret_cast<const sockaddr *>(&target), sizeof(target));
        if (connectResult < 0)
        {
            const int socketError = remoteSocketError(newFd);
            MC_LOG_WARN("network",
                        "[PS2] ps2ips blocking connect failed fd=%d result=%d so_error=%d\n",
                        newFd, connectResult, socketError);
            releaseSocket();
            return false;
        }

        int nonBlocking = 1;
        if (ps2ipc_ioctl(newFd, kIopFionbio, &nonBlocking) < 0)
        {
            MC_LOG_WARN("network",
                        "[PS2] ps2ips FIONBIO failed after connect fd=%d\n", newFd);
            releaseSocket();
            return false;
        }

        return finishRemoteConnect(newFd, host, port);
    }

    bool finishRemoteConnect(int socketFd, const std::string &host, int port)
    {
        const int noDelay = 1;
        if (ps2ipc_setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY,
                              &noDelay, sizeof(noDelay)) < 0)
            MC_LOG_DEBUG("network", "[PS2] ps2ips TCP_NODELAY unavailable fd=%d\n", socketFd);

        MC_LOG_INFO("network", "[PS2] TCP connected to %s:%d via ps2ips fd=%d\n",
                    host.c_str(), port, socketFd);
        McLog::flush();
        return true;
    }
#endif

    void releaseSocket()
    {
        closing.store(true, std::memory_order_release);
        const int socketFd = fd.exchange(-1, std::memory_order_acq_rel);
        if (socketFd < 0)
            return;
#ifdef PS2_REMOTE_DEBUG
        (void)ps2ipc_disconnect(socketFd);
#else
        ::close(socketFd);
#endif
    }

    std::atomic<int> fd{-1};
    std::atomic_bool closing{true};
    std::atomic<std::size_t> receivedBytes{0};
    std::atomic<std::size_t> sentBytes{0};
    PlatformMutex writeLock;
    std::string remoteAddress;
};

class SocketInputBuffer final : public std::streambuf
{
public:
    explicit SocketInputBuffer(Socket &value) : socket(value) { setg(buffer, buffer, buffer); }

protected:
    int_type underflow() override
    {
        if (gptr() < egptr())
            return traits_type::to_int_type(*gptr());
        const int count = socket.read(buffer, sizeof(buffer));
        if (count <= 0)
            return traits_type::eof();
        setg(buffer, buffer, buffer + count);
        return traits_type::to_int_type(*gptr());
    }

private:
    Socket &socket;
    char buffer[1024];
};

class SocketOutputBuffer final : public std::streambuf
{
public:
    explicit SocketOutputBuffer(Socket &value) : socket(value) { setp(buffer, buffer + sizeof(buffer)); }
    ~SocketOutputBuffer() override { sync(); }

protected:
    std::streamsize xsputn(const char *data, std::streamsize length) override
    {
        std::streamsize written = 0;
        while (written < length)
        {
            std::streamsize space = epptr() - pptr();
            if (space == 0)
            {
                if (!flushBuffer())
                    return written;
                space = epptr() - pptr();
            }
            const std::streamsize remaining = length - written;
            const std::streamsize count = remaining < space ? remaining : space;
            std::memcpy(pptr(), data + written, static_cast<std::size_t>(count));
            pbump(static_cast<int>(count));
            written += count;
        }
        return written;
    }

    int_type overflow(int_type value) override
    {
        if (traits_type::eq_int_type(value, traits_type::eof()))
            return traits_type::not_eof(value);
        if (!flushBuffer())
            return traits_type::eof();
        *pptr() = traits_type::to_char_type(value);
        pbump(1);
        return value;
    }

    int sync() override
    {
        return flushBuffer() && socket.flush() ? 0 : -1;
    }

private:
    bool flushBuffer()
    {
        const std::streamsize count = pptr() - pbase();
        if (count > 0 && !socket.write(pbase(), static_cast<int>(count)))
            return false;
        pbump(-static_cast<int>(count));
        return true;
    }

    Socket &socket;
    // Large enough to coalesce packet headers/body writes, small enough to keep
    // two network streams cheap in the PS2's 32 MB main RAM.
    char buffer[4096];
};

class SocketInputStream final : public std::istream
{
public:
    explicit SocketInputStream(Socket &socket) : std::istream(nullptr), buffer(socket) { rdbuf(&buffer); }
private:
    SocketInputBuffer buffer;
};

class SocketOutputStream final : public std::ostream
{
public:
    explicit SocketOutputStream(Socket &socket) : std::ostream(nullptr), buffer(socket) { rdbuf(&buffer); }
private:
    SocketOutputBuffer buffer;
};
}

std::unique_ptr<Socket> createSocket() { return std::make_unique<Ps2Socket>(); }
std::unique_ptr<std::istream> createInputStream(Socket &socket) { return std::make_unique<SocketInputStream>(socket); }
std::unique_ptr<std::ostream> createOutputStream(Socket &socket) { return std::make_unique<SocketOutputStream>(socket); }

// The PS2 multiplayer path only needs raw TCP. Keep HTTP/HTTPS disabled so
// enabling multiplayer does not re-enable desktop resource/auth traffic.
bool readUrl(const std::string &, std::vector<unsigned char> &) { return false; }
int getResponseCode(const std::string &) { return -1; }
bool postUrl(const std::string &, const std::string &, const std::string &,
             std::vector<unsigned char> &) { return false; }
}

#endif
