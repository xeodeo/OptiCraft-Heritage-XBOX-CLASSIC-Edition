#pragma once

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace JavaNetwork
{

class Socket
{
public:
	virtual ~Socket() = default;

	virtual bool connect(const std::string &host, int port) = 0;
	virtual int read(char *buffer, int length) = 0;
	virtual bool write(const char *buffer, int length) = 0;
	virtual bool flush() = 0;
	virtual void interruptRead() { close(); }
	virtual void close() = 0;
	virtual std::string getRemoteSocketAddress() const = 0;
	virtual std::size_t getReceivedByteCount() const { return 0; }
	virtual std::size_t getSentByteCount() const { return 0; }
};

std::unique_ptr<Socket> createSocket();

std::unique_ptr<std::istream> createInputStream(Socket &socket);
std::unique_ptr<std::ostream> createOutputStream(Socket &socket);

enum class HttpResult
{
	Ok,
	NotFound,
	NoConnection,
	InvalidResponse,
	TooLarge
};

// Small platform layer for Java-like URL/HttpURLConnection operations.
// The Minecraft port should call this instead of platform APIs directly.
HttpResult httpGetUrl(const std::string &url,
                      std::vector<unsigned char> &outData,
                      int *outStatusCode = nullptr,
                      std::size_t maxBytes = 64 * 1024,
                      int timeoutMs = 8000);
bool readUrl(const std::string &url, std::vector<unsigned char> &data);
int getResponseCode(const std::string &url);
bool postUrl(const std::string &url, const std::string &contentType,
             const std::string &body, std::vector<unsigned char> &response);

}
