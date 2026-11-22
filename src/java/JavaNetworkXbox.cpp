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

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <cstdlib>
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

#ifdef NO_NETWORK
HttpResult httpGetUrl(const std::string &, std::vector<unsigned char> &outData, int *outStatusCode,
                      std::size_t, int)
{
	outData.clear();
	if (outStatusCode != nullptr)
		*outStatusCode = -1;
	return HttpResult::NoConnection;
}
#else
namespace
{
struct ParsedHttpUrl
{
	std::string host;
	int port = 80;
	std::string path;
};

bool parseHttpUrl(const std::string &url, ParsedHttpUrl &out)
{
	const std::string prefix = "http://";
	if (url.size() <= prefix.size() || url.compare(0, prefix.size(), prefix) != 0)
		return false;

	const std::size_t hostStart = prefix.size();
	const std::size_t pathPos = url.find('/', hostStart);
	std::string authority = (pathPos == std::string::npos) ? url.substr(hostStart) : url.substr(hostStart, pathPos - hostStart);
	out.path = (pathPos == std::string::npos) ? "/" : url.substr(pathPos);
	out.port = 80;

	const std::size_t colonPos = authority.rfind(':');
	if (colonPos != std::string::npos)
	{
		out.host = authority.substr(0, colonPos);
		const int p = std::atoi(authority.substr(colonPos + 1).c_str());
		if (p <= 0 || p > 65535)
			return false;
		out.port = p;
	}
	else
	{
		out.host = authority;
	}
	return !out.host.empty();
}

std::string toLowerAscii(std::string s)
{
	for (char &c : s)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return s;
}

bool decodeChunkedBody(const std::vector<unsigned char> &in, std::vector<unsigned char> &out, std::size_t maxBytes)
{
	out.clear();
	std::size_t pos = 0;
	const std::size_t n = in.size();
	while (pos < n)
	{
		std::size_t lineEnd = pos;
		while (lineEnd + 1 < n && !(in[lineEnd] == '\r' && in[lineEnd + 1] == '\n'))
			++lineEnd;
		if (lineEnd + 1 >= n)
			return false;

		std::size_t chunkSize = 0;
		bool anyHex = false;
		for (std::size_t i = pos; i < lineEnd; ++i)
		{
			const unsigned char c = in[i];
			if (c == ';')
				break;
			if (c >= '0' && c <= '9') { chunkSize = (chunkSize << 4) | (c - '0'); anyHex = true; }
			else if (c >= 'a' && c <= 'f') { chunkSize = (chunkSize << 4) | (c - 'a' + 10); anyHex = true; }
			else if (c >= 'A' && c <= 'F') { chunkSize = (chunkSize << 4) | (c - 'A' + 10); anyHex = true; }
			else if (c == ' ' || c == '\t') continue;
			else return false;
		}
		if (!anyHex)
			return false;
		pos = lineEnd + 2;
		if (chunkSize == 0)
			return true;
		if (pos + chunkSize > n)
			return false;
		if (out.size() + chunkSize > maxBytes)
			return false;
		out.insert(out.end(), in.begin() + static_cast<std::ptrdiff_t>(pos),
		           in.begin() + static_cast<std::ptrdiff_t>(pos + chunkSize));
		pos += chunkSize;
		if (pos + 2 <= n && in[pos] == '\r' && in[pos + 1] == '\n')
			pos += 2;
	}
	return true;
}
} // namespace

HttpResult httpGetUrl(const std::string &url, std::vector<unsigned char> &outData, int *outStatusCode,
                      std::size_t maxBytes, int timeoutMs)
{
	outData.clear();
	if (outStatusCode != nullptr)
		*outStatusCode = -1;

	if (timeoutMs < 1000) timeoutMs = 1000;
	if (timeoutMs > 8000) timeoutMs = 8000;
	if (maxBytes == 0) maxBytes = 64 * 1024;

	const unsigned int startMs = XboxNetwork::tickMs();
	auto remainingMs = [&]() -> int {
		const unsigned int elapsed = XboxNetwork::tickMs() - startMs;
		return (elapsed >= static_cast<unsigned int>(timeoutMs)) ? 0 : (timeoutMs - static_cast<int>(elapsed));
	};

	std::string currentUrl = url;
	for (int redirect = 0; redirect < 4; ++redirect)
	{
		ParsedHttpUrl parsed;
		if (!parseHttpUrl(currentUrl, parsed))
			return HttpResult::InvalidResponse;

		int rem = remainingMs();
		if (rem <= 0)
			return HttpResult::NoConnection;

		const intptr_t sock = XboxNetwork::tcpConnectTimeout(parsed.host.c_str(), parsed.port, rem);
		if (sock == XboxNetwork::kInvalidSocket)
		{
			MC_LOG_INFO("network", "httpGetUrl connect failed: %s:%d (wsa=%d)\n",
			            parsed.host.c_str(), parsed.port, XboxNetwork::lastError());
			return HttpResult::NoConnection;
		}

		std::string req;
		req.reserve(256 + parsed.path.size());
		req += "GET ";
		req += parsed.path;
		req += " HTTP/1.0\r\nHost: ";
		req += parsed.host;
		if (parsed.port != 80)
		{
			req += ":";
			req += std::to_string(parsed.port);
		}
		req += "\r\nUser-Agent: OptiCraft-Xbox/1.0\r\nAccept: image/png,*/*\r\nConnection: close\r\n\r\n";

		std::size_t sent = 0;
		bool sendOk = true;
		while (sent < req.size())
		{
			rem = remainingMs();
			if (rem <= 0)
			{
				sendOk = false;
				break;
			}
			const int n = XboxNetwork::tcpSendTimeout(sock, req.data() + sent,
			                                          static_cast<int>(req.size() - sent), rem);
			if (n <= 0)
			{
				sendOk = false;
				break;
			}
			sent += static_cast<std::size_t>(n);
		}

		if (!sendOk)
		{
			XboxNetwork::tcpShutdown(sock, false);
			XboxNetwork::tcpClose(sock);
			return HttpResult::NoConnection;
		}

		constexpr std::size_t kMaxHeaderBytes = 8192;
		const std::size_t maxRawBytes = maxBytes + kMaxHeaderBytes;
		std::vector<unsigned char> raw;
		raw.reserve(4096);
		char chunk[2048];
		bool tooLarge = false;
		bool timedOut = false;

		for (;;)
		{
			rem = remainingMs();
			if (rem <= 0)
			{
				timedOut = true;
				break;
			}
			const int n = XboxNetwork::tcpRecvTimeout(sock, chunk, sizeof(chunk), rem);
			if (n == 0)
				break;
			if (n < 0)
			{
				if (raw.empty())
					timedOut = true;
				break;
			}
			if (raw.size() + static_cast<std::size_t>(n) > maxRawBytes)
			{
				tooLarge = true;
				break;
			}
			raw.insert(raw.end(), reinterpret_cast<unsigned char *>(chunk),
			           reinterpret_cast<unsigned char *>(chunk + n));
		}

		XboxNetwork::tcpShutdown(sock, false);
		XboxNetwork::tcpClose(sock);

		if (tooLarge)
			return HttpResult::TooLarge;
		if (timedOut && raw.empty())
			return HttpResult::NoConnection;

		// Locate end of HTTP header (\r\n\r\n)
		std::size_t headerEnd = std::string::npos;
		for (std::size_t i = 0; i + 3 < raw.size(); ++i)
		{
			if (raw[i] == '\r' && raw[i + 1] == '\n' && raw[i + 2] == '\r' && raw[i + 3] == '\n')
			{
				headerEnd = i;
				break;
			}
		}
		if (headerEnd == std::string::npos)
			return timedOut ? HttpResult::NoConnection : HttpResult::InvalidResponse;

		std::string headerStr(reinterpret_cast<const char *>(raw.data()), headerEnd);
		const std::size_t firstLineEnd = headerStr.find("\r\n");
		const std::string statusLine = (firstLineEnd == std::string::npos) ? headerStr : headerStr.substr(0, firstLineEnd);

		int statusCode = -1;
		const std::size_t sp1 = statusLine.find(' ');
		if (sp1 != std::string::npos)
			statusCode = std::atoi(statusLine.c_str() + sp1 + 1);
		if (outStatusCode != nullptr)
			*outStatusCode = statusCode;

		// Parse headers for Location and Transfer-Encoding
		std::string location;
		bool isChunked = false;
		if (firstLineEnd != std::string::npos)
		{
			std::size_t pos = firstLineEnd + 2;
			while (pos < headerStr.size())
			{
				std::size_t next = headerStr.find("\r\n", pos);
				std::string line = (next == std::string::npos) ? headerStr.substr(pos) : headerStr.substr(pos, next - pos);
				const std::size_t colon = line.find(':');
				if (colon != std::string::npos)
				{
					std::string key = toLowerAscii(line.substr(0, colon));
					std::string val = line.substr(colon + 1);
					while (!val.empty() && (val.front() == ' ' || val.front() == '\t'))
						val.erase(val.begin());
					while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r'))
						val.pop_back();
					if (key == "location")
						location = val;
					else if (key == "transfer-encoding" && toLowerAscii(val).find("chunked") != std::string::npos)
						isChunked = true;
				}
				if (next == std::string::npos)
					break;
				pos = next + 2;
			}
		}

		if ((statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) &&
		    !location.empty())
		{
			if (location.rfind("http://", 0) == 0)
			{
				currentUrl = location;
				continue;
			}
			if (location[0] == '/')
			{
				currentUrl = "http://" + parsed.host + (parsed.port != 80 ? (":" + std::to_string(parsed.port)) : "") + location;
				continue;
			}
			return HttpResult::InvalidResponse;
		}

		if (statusCode == 404 || statusCode == 400 || statusCode == 410 || statusCode == 422)
			return HttpResult::NotFound;

		if (statusCode < 200 || statusCode >= 300)
			return HttpResult::InvalidResponse;

		const std::size_t bodyStart = headerEnd + 4;
		if (bodyStart > raw.size())
			return HttpResult::InvalidResponse;

		std::vector<unsigned char> body(raw.begin() + static_cast<std::ptrdiff_t>(bodyStart), raw.end());
		if (isChunked)
		{
			if (!decodeChunkedBody(body, outData, maxBytes))
				return HttpResult::InvalidResponse;
		}
		else
		{
			if (body.size() > maxBytes)
				return HttpResult::TooLarge;
			outData = std::move(body);
		}
		return HttpResult::Ok;
	}

	return HttpResult::InvalidResponse;
}
#endif

bool readUrl(const std::string &url, std::vector<unsigned char> &data)
{
	return httpGetUrl(url, data, nullptr, 256 * 1024, 8000) == HttpResult::Ok;
}

int getResponseCode(const std::string &url)
{
	std::vector<unsigned char> data;
	int status = -1;
	httpGetUrl(url, data, &status, 64 * 1024, 8000);
	return status;
}

bool postUrl(const std::string &, const std::string &, const std::string &, std::vector<unsigned char> &)
{
	return false;
}

} // namespace JavaNetwork

#endif // XBOX_PLATFORM
