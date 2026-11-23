#pragma once

#include "java/JavaNetwork.h"

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

// Streaming HTTP(S) GET for big downloads (texture pack zips): the body goes
// to a sink in pieces as it arrives, so nothing has to fit in RAM, with a
// cancel flag and per-read timeouts instead of httpGetUrl's 8 s total.
// Follows redirects (cdn.modrinth.com sends none, GitHub-hosted files do),
// plain or chunked bodies. Blocking: call it from a worker thread.
// Implemented by the Xbox only (JavaNetworkXbox.cpp).
namespace JavaNetwork
{
struct HttpStreamOptions
{
    int connectTimeoutMs = 15000;   // DNS + TCP + TLS handshake
    int idleTimeoutMs = 20000;      // longest wait for the next bytes
    const std::atomic<bool> *cancel = nullptr;
    const char *accept = "*/*";
};

class HttpStreamSink
{
public:
    virtual ~HttpStreamSink() = default;
    // Final (non-redirect) answer: status 2xx and Content-Length or -1.
    // Return false to stop (e.g. the file is bigger than allowed).
    virtual bool onResponse(int status, long long contentLength)
    {
        (void)status;
        (void)contentLength;
        return true;
    }
    // Return false to stop (cancel, disk full...).
    virtual bool onData(const unsigned char *data, std::size_t length) = 0;
};

// Ok when the whole body reached the sink. NotFound for 4xx, InvalidResponse
// for other statuses / broken answers / a sink that said stop, NoConnection
// for network trouble, timeouts and cancel (check your cancel flag).
HttpResult httpGetStream(const std::string &url, HttpStreamSink &sink, int *outStatusCode,
                         const HttpStreamOptions &options);

// Body to memory, at most maxBytes (TooLarge past that). For JSON answers and
// icons with longer timeouts than httpGetUrl.
HttpResult httpGetToMemory(const std::string &url, std::vector<unsigned char> &out, std::size_t maxBytes,
                           int *outStatusCode, const HttpStreamOptions &options);
}
