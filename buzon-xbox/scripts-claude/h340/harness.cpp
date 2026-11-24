// Desktop harness: drives translator340 against a real 1.12.2 server while
// pretending to be the native 1.2.5 client (echoes keepalive/teleports,
// walks a bit, chats, digs). Prints a summary of translated native packets.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <zlib.h>
#include "ProtocolTranslator340.h"

using namespace translator340;

static SOCKET g_s;
static FILE *g_rec = nullptr;
static bool g_quiet = false;
static Session g_sess;
static std::map<int, int> g_counts;
static int g_chunks = 0, g_chunkBytes = 0, g_blocks[256];

static bool recvAll(uint8_t *d, size_t n)
{
	size_t got = 0;
	while (got < n)
	{
		int r = recv(g_s, (char *)d + got, (int)(n - got), 0);
		if (r <= 0)
			return false;
		got += r;
	}
	return true;
}

static bool readVar(uint32_t &v)
{
	v = 0;
	for (int i = 0; i < 5; i++)
	{
		uint8_t b;
		if (!recvAll(&b, 1))
			return false;
		v |= (uint32_t)(b & 0x7f) << (7 * i);
		if (!(b & 0x80))
			return true;
	}
	return false;
}

static void sendFrame(uint32_t id, const std::vector<uint8_t> &payload)
{
	Writer body;
	body.varint(id);
	body.raw(payload);
	Writer frame;
	int th = g_sess.compressionThreshold.load();
	if (th >= 0)
	{
		if ((int)body.b.size() >= th)
		{
			uLongf bound = compressBound((uLong)body.b.size());
			std::vector<uint8_t> comp(bound);
			compress2(comp.data(), &bound, body.b.data(), (uLong)body.b.size(), 6);
			comp.resize(bound);
			Writer inner;
			inner.varint((uint32_t)body.b.size());
			inner.raw(comp);
			frame.varint((uint32_t)inner.b.size());
			frame.raw(inner.b);
		}
		else
		{
			Writer inner;
			inner.varint(0);
			inner.raw(body.b);
			frame.varint((uint32_t)inner.b.size());
			frame.raw(inner.b);
		}
	}
	else
	{
		frame.varint((uint32_t)body.b.size());
		frame.raw(body.b);
	}
	send(g_s, (const char *)frame.b.data(), (int)frame.b.size(), 0);
}

static void flushInjected()
{
	for (;;)
	{
		std::pair<uint32_t, std::vector<uint8_t>> p;
		{
			std::lock_guard<std::mutex> g(g_sess.lock);
			if (g_sess.injected.empty())
				return;
			p = std::move(g_sess.injected.front());
			g_sess.injected.pop_front();
		}
		printf("  >> inject 0x%02x (%zu bytes)\n", p.first, p.second.size());
		sendFrame(p.first, p.second);
	}
}

static void sendNative(uint8_t nid, const Writer &w)
{
	flushInjected();
	std::vector<std::pair<uint32_t, std::vector<uint8_t>>> out;
	translateServerbound(g_sess, nid, w.b.data(), w.b.size(), out);
	for (auto &p : out)
		sendFrame(p.first, p.second);
}

static double px, py, pz;
static bool g_haveTeleport = false;

static void handleNative(const NativePacket &np)
{
	g_counts[np.id]++;
	Cursor c(np.payload.data(), np.payload.size());
	switch (np.id)
	{
	case 0x00:
	{
		Writer w;
		w.i32(c.i32());
		sendNative(0x00, w);
		break;
	}
	case 0x02:
	{
		printf("native handshake '%s' -> sending Packet1Login\n", c.str125(64).c_str());
		Writer w;
		w.i32(29);
		w.str125("Tester", 16);
		w.str125("", 16);
		w.i32(0);
		w.i32(0);
		w.u8(0);
		w.u8(0);
		w.u8(0);
		sendNative(0x01, w);
		break;
	}
	case 0x01:
		printf("native login eid=%d\n", c.i32());
		break;
	case 0x03:
		printf("CHAT: %s\n", c.str125(119).c_str());
		break;
	case 0x0D:
	{
		px = c.f64();
		double y = c.f64();
		double stance = c.f64();
		pz = c.f64();
		py = y < stance ? y : stance;
		printf("teleport to %.2f %.2f %.2f\n", px, py, pz);
		Writer w;
		w.f64(px);
		w.f64(py);
		w.f64(py + 1.62);
		w.f64(pz);
		w.f32(0);
		w.f32(0);
		w.u8(1);
		sendNative(0x0D, w);
		g_haveTeleport = true;
		break;
	}
	case 0xFF:
		printf("KICK: %s\n", c.str125(256).c_str());
		break;
	case 0x18:
	{
		int eid = c.i32();
		int type = c.u8();
		printf("mob spawn eid=%d type=%d payload=%zu\n", eid, type, np.payload.size());
		break;
	}
	case 0x14:
	{
		int eid = c.i32();
		printf("player spawn eid=%d name=%s\n", eid, c.str125(16).c_str());
		break;
	}
	case 0x82:
	{
		int x = c.i32(), y = c.i16(), z = c.i32();
		printf("sign %d %d %d: %s\n", x, y, z, c.str125(15).c_str());
		break;
	}
	default:
		break;
	}
}

int main(int argc, char **argv)
{
	const char *host = argc > 1 ? argv[1] : "127.0.0.1";
	int port = argc > 2 ? atoi(argv[2]) : 25566;
	int seconds = argc > 3 ? atoi(argv[3]) : 20;
	if (argc > 4) g_rec = fopen(argv[4], "wb");
	if (argc > 5) g_quiet = true;
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
	addrinfo hints = {}, *res = nullptr;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, std::to_string(port).c_str(), &hints, &res) != 0)
	{
		printf("dns fail\n");
		return 1;
	}
	g_s = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(g_s, res->ai_addr, (int)res->ai_addrlen) != 0)
	{
		printf("connect fail\n");
		return 1;
	}
	DWORD tv = 200;
	setsockopt(g_s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
	g_log = [](const char *t) { printf("%s\n", t); };
	g_sess.username = "Tester";
	auto hs = encodeHandshake(host, (uint16_t)port, 2);
	auto ls = encodeLoginStart("Tester");
	send(g_s, (const char *)hs.data(), (int)hs.size(), 0);
	send(g_s, (const char *)ls.data(), (int)ls.size(), 0);
	g_sess.stage.store((int)Session::Stage::Login);

	auto t0 = std::chrono::steady_clock::now();
	auto lastTick = t0;
	int tick = 0;
	bool chatted = false;
	double totalConvertMs = 0;
	while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(seconds))
	{
		uint32_t total;
		uint8_t first;
		int r = recv(g_s, (char *)&first, 1, 0);
		if (r == 0)
		{
			printf("closed by server\n");
			break;
		}
		if (r > 0)
		{
			total = first & 0x7f;
			if (first & 0x80)
			{
				uint32_t rest;
				if (!readVar(rest))
					break;
				total |= rest << 7;
			}
			std::vector<uint8_t> frame(total);
			if (!recvAll(frame.data(), total))
				break;
			Cursor c(frame.data(), frame.size());
			std::vector<uint8_t> plain;
			if (g_sess.compressionThreshold.load() >= 0)
			{
				uint32_t dl = c.varint();
				if (dl)
				{
					plain.resize(dl);
					uLongf a = dl;
					if (uncompress(plain.data(), &a, c.p, (uLong)c.left) != Z_OK)
					{
						printf("inflate fail\n");
						break;
					}
					c = Cursor(plain.data(), plain.size());
				}
			}
			if (g_rec) { uint32_t L = (uint32_t)c.left; fwrite(&L, 4, 1, g_rec); fwrite(c.p, 1, c.left, g_rec); fflush(g_rec); }
			uint32_t id = c.varint();
			ClientboundOut out;
			auto a = std::chrono::steady_clock::now();
			translateClientbound(g_sess, id, c.p, c.left, out);
			auto b = std::chrono::steady_clock::now();
			if (id == 0x20)
				totalConvertMs += std::chrono::duration<double, std::milli>(b - a).count();
			size_t pi = 0;
			for (size_t ci = 0; ci <= out.chunks.size(); ci++)
			{
				size_t upto = ci < out.chunks.size() ? out.chunkAt[ci] : out.packets.size();
				for (; pi < upto; pi++)
					handleNative(out.packets[pi]);
				if (ci < out.chunks.size())
				{
					auto &ch = out.chunks[ci];
					g_chunks++;
					g_chunkBytes += (int)ch.data.size();
					int n = 0;
					for (int s = 0; s < 16; s++)
						if (ch.primary & (1 << s))
							n++;
					size_t need = n * 10240 + (ch.groundUp ? 256 : 0);
					if (ch.data.size() != need)
						printf("CHUNK SIZE MISMATCH\n");
					for (int i = 0; i < n * 4096; i++)
						g_blocks[(uint8_t)ch.data[i]]++;
				}
			}
		}
		flushInjected();
		auto now = std::chrono::steady_clock::now();
		if (g_haveTeleport && now - lastTick > std::chrono::milliseconds(50))
		{
			lastTick = now;
			tick++;
			Writer w;
			if (!g_quiet && tick > 60 && tick < 100)
				px += 0.1;
			w.f64(px);
			w.f64(py);
			w.f64(py + 1.62);
			w.f64(pz);
			w.u8(1);
			sendNative(0x0B, w);
			if (!g_quiet && !chatted && tick == 40)
			{
				chatted = true;
				Writer cw;
				cw.str125("hola desde el traductor 1.2.5->1.12.2", 100);
				sendNative(0x03, cw);
			}
			if (!g_quiet && tick == 120)
			{
				// break the block under our feet (survival: start + finish)
				for (int st : {0, 2})
				{
					Writer d;
					d.u8((uint8_t)st);
					d.i32((int)floor(px));
					d.u8((uint8_t)((int)floor(py) - 1));
					d.i32((int)floor(pz));
					d.i8(1);
					sendNative(0x0E, d);
				}
			}
		}
	}
	printf("--- summary ---\nchunks=%d bytes=%d convert=%.1f ms total (%.2f ms/chunk)\n", g_chunks, g_chunkBytes,
	       totalConvertMs, g_chunks ? totalConvertMs / g_chunks : 0.0);
	for (auto &kv : g_counts)
		printf("native 0x%02x x%d\n", kv.first, kv.second);
	printf("top blocks:");
	for (int i = 0; i < 256; i++)
		if (g_blocks[i] > 2000)
			printf(" %d:%d", i, g_blocks[i]);
	printf("\n");
	closesocket(g_s);
	return 0;
}
