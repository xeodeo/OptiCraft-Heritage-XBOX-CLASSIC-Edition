#pragma once

// ProtocolTranslator340 -- 1.2.5 (protocol 29, pre-Netty) <-> 1.12.2
// (protocol 340, Netty framing) translation core.
//
// Lets the 1.2.5 engine join 1.12.2 servers (Paper/Spigot, offline-mode)
// without touching the rest of the client: the IO layer
// (ProtocolTranslator340IO) feeds every 1.12.2 frame through
// translateClientbound() and decodes the resulting native packets with the
// regular Packet factory, and every packet the engine sends goes through
// translateServerbound().
//
// Architecture follows erluza's 1.2.5 <-> 1.8.9 translator
// (ProtocolTranslator189, OptiCraft Heritage Edition): byte-level, no engine
// dependencies, so it can be exercised by a desktop harness against a real
// server. Packet layouts from wiki.vg (protocol 340) and plushmonkey/mclib.
//
// Chunks: 1.12 sends paletted sections; they are decoded once, straight into
// the native Packet51 plane-major layout (ids, metadata, block light, sky
// light, biomes), which the IO layer hands to Packet51MapChunk without any
// zlib round trip. Blocks and items that do not exist in 1.2.5 become the
// closest native one (mapBlock / mapItem).

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace translator340
{

constexpr int kNativeProtocol = 29;
constexpr int kTargetProtocol = 340;

// Optional diagnostics sink (the game installs MC_LOG; the harness printf).
extern void (*g_log)(const char *text);

struct Cursor
{
	const uint8_t *p;
	std::size_t left;
	bool ok = true;

	Cursor(const uint8_t *data, std::size_t len) : p(data), left(len) {}

	uint8_t u8();
	int8_t i8();
	uint16_t u16();
	int16_t i16();
	int32_t i32();
	int64_t i64();
	float f32();
	double f64();
	uint32_t varint();
	int32_t svarint() { return static_cast<int32_t>(varint()); }
	std::string str125(std::size_t maxChars); // short count + UTF-16BE -> UTF-8
	std::string strMc();                      // VarInt byte count + UTF-8
	void skip(std::size_t n);
};

struct Writer
{
	std::vector<uint8_t> b;
	void u8(uint8_t v) { b.push_back(v); }
	void i8(int8_t v) { b.push_back(static_cast<uint8_t>(v)); }
	void u16(uint16_t v);
	void i16(int16_t v) { u16(static_cast<uint16_t>(v)); }
	void i32(int32_t v);
	void i64(int64_t v);
	void f32(float v);
	void f64(double v);
	void varint(uint32_t v);
	void raw(const uint8_t *data, std::size_t len) { b.insert(b.end(), data, data + len); }
	void raw(const std::vector<uint8_t> &v) { b.insert(b.end(), v.begin(), v.end()); }
	void str125(const std::string &utf8, std::size_t maxChars);
	void strMc(const std::string &utf8);
};

int64_t encodePosition(int32_t x, int32_t y, int32_t z);
void decodePosition(int64_t v, int32_t &x, int32_t &y, int32_t &z);

// 1.12 JSON chat component -> legacy text with section colour codes.
std::string chatToText(const std::string &json);
std::string truncateUtf8(const std::string &text, std::size_t maxCodePoints);

// 1.12 block state (id, meta) -> closest 1.2.5 block. Ids above 124 and the
// metadata values 1.2.5 does not know are folded into native ones.
void mapBlock(uint32_t id, uint32_t meta, uint8_t &outId, uint8_t &outMeta);
// 1.12 biome id -> 1.2.5 biome id (0..22).
uint8_t mapBiome(uint8_t biome);
// 1.12 item id (+damage) -> native id (-1 = not representable, empty slot).
int16_t mapItem(int16_t id, int16_t &damage);

// Native registry probes installed by the IO layer (null in the harness).
extern bool (*g_nativeItemExists)(int16_t id);
// Block registry probe: lets the same core serve the 1.2.5 engine and the
// 1.5.2 engine (which knows blocks up to 158).
extern bool (*g_nativeBlockExists)(int id);
extern bool (*g_nativeItemWantsNbt)(int16_t id);
extern bool (*g_gzipCompress)(const uint8_t *in, std::size_t len, std::vector<uint8_t> &out);
extern bool (*g_gzipDecompress)(const uint8_t *in, std::size_t len, std::vector<uint8_t> &out);

std::size_t skipNbt(const uint8_t *p, std::size_t len);
// Length of one tag payload of the given type (0 on malformed input).
std::size_t skipNbtPayload(const uint8_t *p, std::size_t len, uint8_t tag);

bool convertSlotToNative(Cursor &c, Writer &w);
bool convertSlotFromNative(Cursor &c, Writer &w);

// One decoded 1.12 column in the native Packet51 layout (uncompressed).
struct ConvertedChunk
{
	int32_t x = 0;
	int32_t z = 0;
	bool groundUp = false;
	uint16_t primary = 0;
	std::vector<int8_t> data; // ids, metas, block light, sky light, [biomes] (signed: Packet51's byte_t)
};

bool convertChunkColumn(const uint8_t *data, std::size_t len, uint32_t mask, bool groundUp,
                        bool skylight, ConvertedChunk &out);

struct NativePacket
{
	uint8_t id = 0;
	std::vector<uint8_t> payload;
};

enum class EntityKind : uint8_t
{
	Unknown = 0,
	Player,
	Living,
	Ageable,
	Pig,
	Sheep,
	Wolf,
	Ocelot,
	Creeper,
	Slime,
	Ghast,
	Spider,
	Blaze,
	Enderman,
	Villager,
	IronGolem,
	Dragon,
	Boat,
	Minecart,
	Item,
	Potion,
};

struct EntityTrack
{
	EntityKind kind = EntityKind::Unknown;
	int64_t x = 0, y = 0, z = 0;  // exact position, 1/4096 block
	int32_t nx = 0, ny = 0, nz = 0; // position the native client holds, 1/32 block
	bool pending = false;         // item/potion spawn waiting for its metadata
	bool potion = false;
	int8_t yaw = 0, pitch = 0;
	int16_t vx = 0, vy = 0, vz = 0;
};

struct Session
{
	enum class Stage
	{
		Idle,
		Login,
		Play,
	};

	std::atomic<int> stage{static_cast<int>(Stage::Idle)};
	std::atomic<int> compressionThreshold{-1};
	std::atomic<int32_t> entityId{-1};
	std::atomic<int> dimension{0};
	std::string username;

	// Reader-thread only.
	std::map<int32_t, EntityTrack> entities;
	std::map<std::string, std::string> playerNames; // 16-byte uuid -> name
	std::map<int32_t, std::vector<int32_t>> passengers;
	std::map<int, int> windowKinds; // window id -> native inventory type

	// Shared reader/writer.
	std::mutex lock;
	double posX = 0.0, posY = 0.0, posZ = 0.0;
	float yaw = 0.0f, pitch = 0.0f;
	int64_t keepAlive[4] = {0, 0, 0, 0};
	bool keepAlivePending[4] = {false, false, false, false};
	int keepAliveNext = 0;
	// Serverbound frames the reader needs sent (teleport confirm, client
	// settings, window close): packet id + payload, framed by the IO layer.
	std::deque<std::pair<uint32_t, std::vector<uint8_t>>> injected;

	void inject(uint32_t id, const Writer &w);
};

// Login open: complete uncompressed frames.
std::vector<uint8_t> encodeHandshake(const std::string &host, uint16_t port, uint32_t nextState);
std::vector<uint8_t> encodeLoginStart(const std::string &username);

// Native packet (id + payload) -> zero or more (id, payload) 1.12 packets.
// Returns false when the packet was dropped.
bool translateServerbound(Session &s, uint8_t nativeId, const uint8_t *payload, std::size_t len,
                          std::vector<std::pair<uint32_t, std::vector<uint8_t>>> &out);

struct ClientboundOut
{
	std::vector<NativePacket> packets;
	std::vector<ConvertedChunk> chunks;
	// Order: chunk i must be delivered before packets[chunkAt[i]].
	std::vector<std::size_t> chunkAt;
};
void translateClientbound(Session &s, uint32_t id, const uint8_t *payload, std::size_t len,
                          ClientboundOut &out);

// Op permission level the server gave the local player (Entity Status 24..28),
// -1 until it says; reset on every login. The admin mode mod reads it.
int localOpLevel();
void resetLocalOpLevel();

bool statusToNative(const std::string &statusJson, std::string &motdOut, int &onlineOut, int &maxOut,
                    int *protocolOut = nullptr);

} // namespace translator340
