#pragma once

#include <cstdint>

// TLS 1.2 client over an already connected XboxNetwork socket (mbedTLS, same
// library and configuration as the Classic Xbox Store).
//
// "Relaxed" HTTPS: the connection is encrypted and sends SNI, but the server's
// certificate chain is NOT verified (loading a root store would cost hundreds of
// KB of RAM). Good enough to download texture packs and skins; never use it for
// passwords or anything private.
//
// Blocking calls: use them from a worker thread, not the render thread.
namespace XboxTls
{
struct Session;

// Runs the handshake. Returns nullptr on failure (`error` gets a short reason).
// The socket stays owned by the caller (close it after close()).
Session *open(intptr_t socket, const char *host, int timeoutMs, char *error, int errorSize);

// > 0 bytes, < 0 error. Sends until everything is written or it fails.
int send(Session *session, const char *data, int length, int timeoutMs);

// > 0 bytes, 0 the peer closed, < 0 error or timeout.
int recv(Session *session, char *buffer, int length, int timeoutMs);

void close(Session *session);
}
