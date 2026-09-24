// JavaNetworkXbox.cpp — java/JavaNetwork.h for the Xbox.
//
// With networking enabled (XBOX_ENABLE_NETWORK, i.e. no NO_NETWORK define)
// sockets are XNet TCP sockets opened through XboxNetwork (security-bypass
// mode, the one that may reach an ordinary Minecraft 1.2.5 server). It mirrors
// src/wii/JavaNetwork_wii.cpp. URL requests are not implemented (skins and
// resources come from the local data).
//
// This file lives outside src/xbox on purpose: sources there are compiled with
// the XDK include path, whose 2003 STL headers shadow the VS2022 ones. Streams
// built against that STL reached NetworkManager (VS2022 STL) with a different
// std::ios layout and failed at once with "ios_base::badbit set". Only plain C
// calls cross into XboxNetwork.cpp.
//
// Without networking the sockets never connect, like the PS2 build.
#ifdef XBOX_PLATFORM

#include "java/JavaNetwork.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <streambuf>

#ifndef NO_NETWORK
#include "platform/Log.h"
#include "xbox/system/XboxNetwork.h"
#endif

namespace JavaNetwork
{

namespace
{
#ifdef NO_NETWORK

class OfflineSocket : public Socket
{
public:
	bool connect(const std::string &, int) override { return false; }
	int read(char *, int) override { return -1; }
	bool write(const char *, int) override { return false; }
	bool flush() override { return false; }
	void close() override {}
	std::string getRemoteSocketAddress() const override { return std::string(); }
};

#else

class XboxSocket final : public Socket
{
public:
	~XboxSocket() override { releaseSocket(); }

	bool connect(const std::string &host, int port) override
	{
		releaseSocket();
		closing.store(false, std::memory_order_release);
		receivedBytes.store(0, std::memory_order_release);
		sentBytes.store(0, std::memory_order_release);
		remoteAddress = host + ":" + std::to_string(port);
		const intptr_t s = XboxNetwork::tcpConnect(host.c_str(), port);
		if (s == XboxNetwork::kInvalidSocket)
		{
			MC_LOG_ERROR("network", "xbox connect %s failed, wsa=%d\n", remoteAddress.c_str(), XboxNetwork::lastError());
			return false;
		}
		fd.store(s, std::memory_order_release);
		return true;
	}

	int read(char *buffer, int length) override
	{
		const intptr_t s = fd.load(std::memory_order_acquire);
		if (s == XboxNetwork::kInvalidSocket || buffer == nullptr || length <= 0 ||
		    closing.load(std::memory_order_acquire))
			return -1;
		const int count = XboxNetwork::tcpRecv(s, buffer, length);
		if (count <= 0 && !closing.load(std::memory_order_acquire))
			MC_LOG_ERROR("network", "xbox recv returned %d, wsa=%d, got %u bytes so far\n", count,
			             XboxNetwork::lastError(), static_cast<unsigned>(receivedBytes.load()));
		if (count > 0)
			receivedBytes.fetch_add(static_cast<std::size_t>(count), std::memory_order_relaxed);
		return count > 0 ? count : -1;
	}

	bool write(const char *buffer, int length) override
	{
		const intptr_t s = fd.load(std::memory_order_acquire);
		if (s == XboxNetwork::kInvalidSocket || buffer == nullptr || closing.load(std::memory_order_acquire))
		{
			MC_LOG_ERROR("network", "xbox write on closed socket (fd=%d closing=%d)\n", static_cast<int>(s),
			             closing.load() ? 1 : 0);
			return false;
		}
		int offset = 0;
		while (offset < length)
		{
			if (closing.load(std::memory_order_acquire))
				return false;
			const int count = XboxNetwork::tcpSend(s, buffer + offset, length - offset);
			if (count <= 0)
			{
				MC_LOG_ERROR("network", "xbox send(%d) returned %d, wsa=%d\n", length - offset, count, XboxNetwork::lastError());
				return false;
			}
			sentBytes.fetch_add(static_cast<std::size_t>(count), std::memory_order_relaxed);
			offset += count;
		}
		return true;
	}

	bool flush() override
	{
		return fd.load(std::memory_order_acquire) != XboxNetwork::kInvalidSocket &&
		       !closing.load(std::memory_order_acquire);
	}

	void interruptRead() override
	{
		const intptr_t s = fd.load(std::memory_order_acquire);
		if (s != XboxNetwork::kInvalidSocket)
			XboxNetwork::tcpShutdown(s, true);
	}

	void close() override
	{
		closing.store(true, std::memory_order_release);
		const intptr_t s = fd.load(std::memory_order_acquire);
		if (s != XboxNetwork::kInvalidSocket)
			XboxNetwork::tcpShutdown(s, false);
	}

	std::string getRemoteSocketAddress() const override { return remoteAddress; }
	std::size_t getReceivedByteCount() const override { return receivedBytes.load(std::memory_order_relaxed); }
	std::size_t getSentByteCount() const override { return sentBytes.load(std::memory_order_relaxed); }

private:
	void releaseSocket()
	{
		closing.store(true, std::memory_order_release);
		const intptr_t s = fd.exchange(XboxNetwork::kInvalidSocket, std::memory_order_acq_rel);
		if (s != XboxNetwork::kInvalidSocket)
		{
			XboxNetwork::tcpShutdown(s, false);
			XboxNetwork::tcpClose(s);
		}
	}

	std::atomic<intptr_t> fd{XboxNetwork::kInvalidSocket};
	std::atomic_bool closing{true};
	std::atomic<std::size_t> receivedBytes{0};
	std::atomic<std::size_t> sentBytes{0};
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
	char buffer[4096];
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

	int sync() override { return flushBuffer() && socket.flush() ? 0 : -1; }

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
	char buffer[5120];
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

#endif // NO_NETWORK
} // namespace

#ifdef NO_NETWORK
std::unique_ptr<Socket> createSocket() { return std::make_unique<OfflineSocket>(); }
std::unique_ptr<std::istream> createInputStream(Socket &) { return std::make_unique<std::istringstream>(); }
std::unique_ptr<std::ostream> createOutputStream(Socket &) { return std::make_unique<std::ostringstream>(); }
#else
std::unique_ptr<Socket> createSocket() { return std::make_unique<XboxSocket>(); }
std::unique_ptr<std::istream> createInputStream(Socket &socket) { return std::make_unique<SocketInputStream>(socket); }
std::unique_ptr<std::ostream> createOutputStream(Socket &socket) { return std::make_unique<SocketOutputStream>(socket); }
#endif

bool readUrl(const std::string &, std::vector<unsigned char> &)
{
	return false;
}

int getResponseCode(const std::string &)
{
	return -1;
}

bool postUrl(const std::string &, const std::string &, const std::string &, std::vector<unsigned char> &)
{
	return false;
}

} // namespace JavaNetwork

#endif // XBOX_PLATFORM
