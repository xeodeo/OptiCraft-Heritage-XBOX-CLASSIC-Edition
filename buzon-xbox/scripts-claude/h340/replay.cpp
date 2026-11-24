// Offline replay: feeds a recorded 1.12.2 session (frames after
// decompression) through translator340 and checks every native packet
// against the protocol-29 reader layouts (consumed == payload size).
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include "ProtocolTranslator340.h"
#include <zlib.h>
#include <chrono>
static size_t g_z = 0, g_raw = 0; static double g_ms = 0;
using namespace translator340;

static bool itemWantsNbt(int16_t id)
{
	// damageable tools/armour in 1.2.5: 256-259, 261, 267-279, 283-286, 290-294, 298-317, 346, 359
	return (id >= 256 && id <= 259) || id == 261 || (id >= 267 && id <= 279) || (id >= 283 && id <= 286) ||
	       (id >= 290 && id <= 294) || (id >= 298 && id <= 317) || id == 346 || id == 359;
}

static bool readStack(Cursor &c)
{
	int16_t id = c.i16();
	if (id < 0)
		return c.ok;
	c.u8();
	c.i16();
	if (itemWantsNbt(id))
	{
		int16_t n = c.i16();
		if (n > 0)
			c.skip(n);
	}
	return c.ok;
}

static bool readWatcher(Cursor &c)
{
	for (int guard = 0; guard < 64; guard++)
	{
		uint8_t k = c.u8();
		if (!c.ok)
			return false;
		if (k == 0x7F)
			return true;
		switch (k >> 5)
		{
		case 0: c.u8(); break;
		case 1: c.i16(); break;
		case 2: c.i32(); break;
		case 3: c.f32(); break;
		case 4: c.str125(64); break;
		case 5: c.i16(); c.u8(); c.i16(); break;
		case 6: c.i32(); c.i32(); c.i32(); break;
		default: return false;
		}
	}
	return false;
}

static std::map<int, int> g_bad, g_count;

static void check(const NativePacket &np)
{
	Cursor c(np.payload.data(), np.payload.size());
	g_count[np.id]++;
	switch (np.id)
	{
	case 0x00: c.i32(); break;
	case 0x01: c.i32(); c.str125(16); c.str125(16); c.i32(); c.i32(); c.u8(); c.u8(); c.u8(); break;
	case 0x02: c.str125(64); break;
	case 0x03: c.str125(119); break;
	case 0x04: c.i64(); break;
	case 0x05: c.i32(); c.i16(); c.i16(); c.i16(); break;
	case 0x06: c.i32(); c.i32(); c.i32(); break;
	case 0x08: c.i16(); c.i16(); c.f32(); break;
	case 0x09: c.i32(); c.u8(); c.u8(); c.i16(); c.str125(16); break;
	case 0x0D: c.skip(32); c.f32(); c.f32(); c.u8(); break;
	case 0x11: c.i32(); c.u8(); c.i32(); c.u8(); c.i32(); break;
	case 0x12: c.i32(); c.u8(); break;
	case 0x14: c.i32(); c.str125(16); c.i32(); c.i32(); c.i32(); c.u8(); c.u8(); c.i16(); break;
	case 0x15: c.i32(); c.i16(); c.u8(); c.i16(); c.skip(15); break;
	case 0x16: c.i32(); c.i32(); break;
	case 0x17:
	{
		c.i32(); c.u8(); c.i32(); c.i32(); c.i32();
		int t = c.i32();
		if (t > 0) c.skip(6);
		break;
	}
	case 0x18: c.i32(); c.u8(); c.i32(); c.i32(); c.i32(); c.u8(); c.u8(); c.u8(); readWatcher(c); break;
	case 0x19: c.i32(); c.str125(13); c.skip(16); break;
	case 0x1A: c.i32(); c.i32(); c.i32(); c.i32(); c.i16(); break;
	case 0x1C: c.i32(); c.skip(6); break;
	case 0x1D: c.i32(); break;
	case 0x1F: c.i32(); c.skip(3); break;
	case 0x20: c.i32(); c.skip(2); break;
	case 0x21: c.i32(); c.skip(5); break;
	case 0x22: c.i32(); c.skip(14); break;
	case 0x23: c.i32(); c.u8(); break;
	case 0x26: c.i32(); c.u8(); break;
	case 0x27: c.i32(); c.i32(); break;
	case 0x28: c.i32(); readWatcher(c); break;
	case 0x29: c.i32(); c.u8(); c.u8(); c.i16(); break;
	case 0x2A: c.i32(); c.u8(); break;
	case 0x2B: c.f32(); c.i16(); c.i16(); break;
	case 0x32: c.i32(); c.i32(); c.u8(); break;
	case 0x34: { c.i32(); c.i32(); int n = (uint16_t)c.i16(); int l = c.i32(); if (l != n * 4) c.ok = false; c.skip(l); break; }
	case 0x35: c.i32(); c.u8(); c.i32(); c.u8(); c.u8(); break;
	case 0x36: c.i32(); c.i16(); c.i32(); c.u8(); c.u8(); break;
	case 0x3C: { c.skip(24); c.f32(); int n = c.i32(); c.skip(n * 3); break; }
	case 0x3D: c.i32(); c.i32(); c.u8(); c.i32(); c.i32(); break;
	case 0x46: c.u8(); c.u8(); break;
	case 0x47: c.i32(); c.u8(); c.i32(); c.i32(); c.i32(); break;
	case 0x64: c.u8(); c.u8(); c.str125(32); c.u8(); break;
	case 0x65: c.u8(); break;
	case 0x67: c.u8(); c.i16(); readStack(c); break;
	case 0x68: { c.u8(); int n = c.i16(); for (int i = 0; i < n && c.ok; i++) readStack(c); break; }
	case 0x69: c.u8(); c.i16(); c.i16(); break;
	case 0x6A: c.u8(); c.i16(); c.u8(); break;
	case 0x82: c.i32(); c.i16(); c.i32(); for (int i = 0; i < 4; i++) c.str125(15); break;
	case 0xC9: c.str125(16); c.u8(); c.i16(); break;
	case 0xCA: c.skip(4); break;
	case 0xFF: c.str125(256); break;
	default: printf("unchecked native 0x%02x\n", np.id); return;
	}
	if (!c.ok || c.left != 0)
	{
		if (g_bad[np.id]++ < 3)
			printf("BAD native 0x%02x size=%zu left=%zu ok=%d\n", np.id, np.payload.size(), c.left, c.ok);
	}
}

int main(int argc, char **argv)
{
	g_nativeItemWantsNbt = &itemWantsNbt;
	FILE *f = fopen(argv[1], "rb");
	if (!f)
		return 1;
	Session s;
	s.username = "Tester";
	s.stage.store((int)Session::Stage::Login);
	int frames = 0;
	size_t maxChunk = 0;
	for (;;)
	{
		uint32_t L;
		if (fread(&L, 4, 1, f) != 1)
			break;
		std::vector<uint8_t> fr(L);
		if (fread(fr.data(), 1, L, f) != L)
			break;
		Cursor c(fr.data(), fr.size());
		uint32_t id = c.varint();
		ClientboundOut out;
		translateClientbound(s, id, c.p, c.left, out);
		for (auto &np : out.packets)
			check(np);
		for (auto &ch : out.chunks)
		{
			uLongf b = compressBound((uLong)ch.data.size()); std::vector<uint8_t> z(b);
			auto t0 = std::chrono::steady_clock::now();
			compress2(z.data(), &b, (const Bytef*)ch.data.data(), (uLong)ch.data.size(), 1);
			g_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
			g_z += b; g_raw += ch.data.size();
		}
		for (auto &ch : out.chunks)
			if (ch.data.size() > maxChunk)
				maxChunk = ch.data.size();
		frames++;
	}
	printf("frames=%d maxChunkBytes=%zu raw=%zu z=%zu deflate=%.1fms\n", frames, maxChunk, g_raw, g_z, g_ms);
	for (auto &kv : g_count)
		printf("0x%02x x%d%s\n", kv.first, kv.second, g_bad[kv.first] ? "  <-- BAD" : "");
	return 0;
}
