// JavaNetwork_xbox.cpp — the Xbox port is built with NO_NETWORK, so this is the
// offline implementation of java/JavaNetwork.h: sockets never connect and URL
// requests fail, the same answers src/ps2/JavaNetwork_ps2.cpp gives for HTTP.
// Multiplayer over the Xbox network stack (XNet) can replace it later.
#ifdef XBOX_PLATFORM

#include "java/JavaNetwork.h"

#include <memory>
#include <sstream>

namespace JavaNetwork
{

namespace
{
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
} // namespace

std::unique_ptr<Socket> createSocket()
{
	return std::make_unique<OfflineSocket>();
}

std::unique_ptr<std::istream> createInputStream(Socket &)
{
	return std::make_unique<std::istringstream>();
}

std::unique_ptr<std::ostream> createOutputStream(Socket &)
{
	return std::make_unique<std::ostringstream>();
}

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
