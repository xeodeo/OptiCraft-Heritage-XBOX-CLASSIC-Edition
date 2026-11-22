#ifdef XBOX_PLATFORM
#include "xbox/system/XboxTls.h"

#ifndef NO_NETWORK

#include "xbox/XboxXtl.h"
#include "xbox/system/XboxNetwork.h"

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"

#include <intrin.h>
#include <stdio.h>
#include <string.h>

// Entropy for mbedTLS (MBEDTLS_ENTROPY_HARDWARE_ALT): the XNet generator mixed with
// the cycle counter, as in the Classic Xbox Store.
extern "C" int mbedtls_hardware_poll(void *, unsigned char *output, size_t length, size_t *produced)
{
    XNetRandom(output, static_cast<UINT>(length));
    for (size_t i = 0; i < length; ++i)
        output[i] ^= static_cast<unsigned char>(__rdtsc() >> ((i & 3) * 2));
    *produced = length;
    return 0;
}

namespace
{
volatile LONG s_lock = 0;
bool s_ready = false;
mbedtls_entropy_context s_entropy;
mbedtls_ctr_drbg_context s_drbg;
mbedtls_ssl_config s_config;

bool initOnce(char *error, int errorSize)
{
    while (InterlockedCompareExchange(&s_lock, 1, 0) != 0)
        Sleep(1);
    if (!s_ready)
    {
        mbedtls_entropy_init(&s_entropy);
        mbedtls_ctr_drbg_init(&s_drbg);
        mbedtls_ssl_config_init(&s_config);
        static const unsigned char kPersonal[] = "OptiCraftXbox";
        int ret = mbedtls_ctr_drbg_seed(&s_drbg, mbedtls_entropy_func, &s_entropy, kPersonal, sizeof(kPersonal));
        if (ret == 0)
            ret = mbedtls_ssl_config_defaults(&s_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                              MBEDTLS_SSL_PRESET_DEFAULT);
        if (ret == 0)
        {
            // Relaxed mode: no root store, the chain is not checked (see XboxTls.h).
            mbedtls_ssl_conf_authmode(&s_config, MBEDTLS_SSL_VERIFY_NONE);
            mbedtls_ssl_conf_rng(&s_config, mbedtls_ctr_drbg_random, &s_drbg);
            s_ready = true;
        }
        else if (error != nullptr && errorSize > 0)
        {
            char text[96];
            mbedtls_strerror(ret, text, sizeof(text));
            _snprintf(error, errorSize, "tls init: %s", text);
            error[errorSize - 1] = 0;
        }
    }
    const bool ready = s_ready;
    InterlockedExchange(&s_lock, 0);
    return ready;
}
} // namespace

namespace XboxTls
{
struct Session
{
    intptr_t socket;
    int timeoutMs;
    mbedtls_ssl_context ssl;
};

namespace
{
int bioSend(void *context, const unsigned char *data, size_t length)
{
    Session *s = static_cast<Session *>(context);
    const int sent = XboxNetwork::tcpSendTimeout(s->socket, reinterpret_cast<const char *>(data),
                                                 static_cast<int>(length), s->timeoutMs);
    return sent <= 0 ? MBEDTLS_ERR_NET_SEND_FAILED : sent;
}

int bioRecv(void *context, unsigned char *data, size_t length)
{
    Session *s = static_cast<Session *>(context);
    const int got = XboxNetwork::tcpRecvTimeout(s->socket, reinterpret_cast<char *>(data),
                                                static_cast<int>(length), s->timeoutMs);
    if (got == 0)
        return MBEDTLS_ERR_NET_CONN_RESET;
    return got < 0 ? MBEDTLS_ERR_NET_RECV_FAILED : got;
}
} // namespace

Session *open(intptr_t socket, const char *host, int timeoutMs, char *error, int errorSize)
{
    if (error != nullptr && errorSize > 0)
        error[0] = 0;
    if (!initOnce(error, errorSize))
        return nullptr;

    Session *s = new Session();
    s->socket = socket;
    s->timeoutMs = timeoutMs > 0 ? timeoutMs : 8000;
    mbedtls_ssl_init(&s->ssl);
    int ret = mbedtls_ssl_setup(&s->ssl, &s_config);
    if (ret == 0)
        ret = mbedtls_ssl_set_hostname(&s->ssl, host);
    if (ret == 0)
    {
        mbedtls_ssl_set_bio(&s->ssl, s, bioSend, bioRecv, nullptr);
        const DWORD start = GetTickCount();
        do
        {
            ret = mbedtls_ssl_handshake(&s->ssl);
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
                break;
        } while (GetTickCount() - start < static_cast<DWORD>(s->timeoutMs));
    }
    if (ret != 0)
    {
        if (error != nullptr && errorSize > 0)
        {
            char text[96];
            mbedtls_strerror(ret, text, sizeof(text));
            _snprintf(error, errorSize, "tls handshake %s: %s", host, text);
            error[errorSize - 1] = 0;
        }
        mbedtls_ssl_free(&s->ssl);
        delete s;
        return nullptr;
    }
    return s;
}

int send(Session *session, const char *data, int length, int timeoutMs)
{
    if (session == nullptr)
        return -1;
    session->timeoutMs = timeoutMs > 0 ? timeoutMs : session->timeoutMs;
    int total = 0;
    while (total < length)
    {
        const int ret = mbedtls_ssl_write(&session->ssl, reinterpret_cast<const unsigned char *>(data + total),
                                          static_cast<size_t>(length - total));
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;
        if (ret <= 0)
            return -1;
        total += ret;
    }
    return total;
}

int recv(Session *session, char *buffer, int length, int timeoutMs)
{
    if (session == nullptr)
        return -1;
    session->timeoutMs = timeoutMs > 0 ? timeoutMs : session->timeoutMs;
    for (;;)
    {
        const int ret = mbedtls_ssl_read(&session->ssl, reinterpret_cast<unsigned char *>(buffer),
                                         static_cast<size_t>(length));
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE)
            continue;
        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == MBEDTLS_ERR_NET_CONN_RESET)
            return 0;
        return ret < 0 ? -1 : ret;
    }
}

void close(Session *session)
{
    if (session == nullptr)
        return;
    mbedtls_ssl_close_notify(&session->ssl);
    mbedtls_ssl_free(&session->ssl);
    delete session;
}
} // namespace XboxTls

#else // NO_NETWORK

namespace XboxTls
{
struct Session {};
Session *open(intptr_t, const char *, int, char *error, int errorSize)
{
    if (error != nullptr && errorSize > 0)
        error[0] = 0;
    return nullptr;
}
int send(Session *, const char *, int, int) { return -1; }
int recv(Session *, char *, int, int) { return -1; }
void close(Session *) {}
}

#endif // NO_NETWORK
#endif // XBOX_PLATFORM
