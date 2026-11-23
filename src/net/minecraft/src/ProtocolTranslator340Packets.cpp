// ProtocolTranslator340: packet tables. Serverbound = native 1.2.5 packet ->
// 1.12.2, clientbound = 1.12.2 packet -> native 1.2.5 packets.
// Packet ids and layouts: wiki.vg "Protocol" (1.12.2, protocol 340),
// cross-checked against plushmonkey/mclib (Protocol_1_12_1 tables).

#include "ProtocolTranslator340.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace translator340
{
namespace
{
std::atomic<int> s_localOpLevel{-1};

// 1.12.2 play, clientbound.
enum Clientbound : uint32_t
{
	cSpawnObject = 0x00,
	cSpawnExpOrb = 0x01,
	cSpawnGlobal = 0x02,
	cSpawnMob = 0x03,
	cSpawnPainting = 0x04,
	cSpawnPlayer = 0x05,
	cAnimation = 0x06,
	cUpdateBlockEntity = 0x09,
	cBlockAction = 0x0A,
	cBlockChange = 0x0B,
	cChat = 0x0F,
	cMultiBlockChange = 0x10,
	cConfirmTransaction = 0x11,
	cCloseWindow = 0x12,
	cOpenWindow = 0x13,
	cWindowItems = 0x14,
	cWindowProperty = 0x15,
	cSetSlot = 0x16,
	cDisconnect = 0x1A,
	cEntityStatus = 0x1B,
	cExplosion = 0x1C,
	cUnloadChunk = 0x1D,
	cChangeGameState = 0x1E,
	cKeepAlive = 0x1F,
	cChunkData = 0x20,
	cEffect = 0x21,
	cJoinGame = 0x23,
	cEntityRelMove = 0x26,
	cEntityLookRelMove = 0x27,
	cEntityLook = 0x28,
	cPlayerAbilities = 0x2C,
	cPlayerListItem = 0x2E,
	cPlayerPosLook = 0x2F,
	cUseBed = 0x30,
	cDestroyEntities = 0x32,
	cRemoveEntityEffect = 0x33,
	cResourcePackSend = 0x34,
	cRespawn = 0x35,
	cEntityHeadLook = 0x36,
	cEntityMetadata = 0x3C,
	cEntityVelocity = 0x3E,
	cEntityEquipment = 0x3F,
	cSetExperience = 0x40,
	cUpdateHealth = 0x41,
	cSetPassengers = 0x43,
	cSpawnPosition = 0x46,
	cTimeUpdate = 0x47,
	cCollectItem = 0x4B,
	cEntityTeleport = 0x4C,
	cEntityEffect = 0x4F,
};

// 1.12.2 play, serverbound.
enum Serverbound : uint32_t
{
	sTeleportConfirm = 0x00,
	sChat = 0x02,
	sClientStatus = 0x03,
	sClientSettings = 0x04,
	sConfirmTransaction = 0x05,
	sEnchantItem = 0x06,
	sClickWindow = 0x07,
	sCloseWindow = 0x08,
	sPluginMessage = 0x09,
	sUseEntity = 0x0A,
	sKeepAlive = 0x0B,
	sPlayer = 0x0C,
	sPlayerPosition = 0x0D,
	sPlayerPosLook = 0x0E,
	sPlayerLook = 0x0F,
	sPlayerAbilities = 0x13,
	sPlayerDigging = 0x14,
	sEntityAction = 0x15,
	sResourcePackStatus = 0x18,
	sHeldItemChange = 0x1A,
	sCreativeAction = 0x1B,
	sUpdateSign = 0x1C,
	sAnimation = 0x1D,
	sBlockPlacement = 0x1F,
	sUseItem = 0x20,
};

// Native 1.2.5 packet ids.
enum Native : uint8_t
{
	nKeepAlive = 0x00,
	nLogin = 0x01,
	nHandshake = 0x02,
	nChat = 0x03,
	nTime = 0x04,
	nPlayerInventory = 0x05,
	nSpawnPos = 0x06,
	nUseEntity = 0x07,
	nHealth = 0x08,
	nRespawn = 0x09,
	nFlying = 0x0A,
	nPos = 0x0B,
	nLook = 0x0C,
	nPosLook = 0x0D,
	nDig = 0x0E,
	nPlace = 0x0F,
	nHeld = 0x10,
	nSleep = 0x11,
	nAnimation = 0x12,
	nEntityAction = 0x13,
	nNamedEntitySpawn = 0x14,
	nPickupSpawn = 0x15,
	nCollect = 0x16,
	nVehicleSpawn = 0x17,
	nMobSpawn = 0x18,
	nPainting = 0x19,
	nExpOrb = 0x1A,
	nVelocity = 0x1C,
	nDestroy = 0x1D,
	nRelMove = 0x1F,
	nLookE = 0x20,
	nRelMoveLook = 0x21,
	nTeleport = 0x22,
	nHeadRotation = 0x23,
	nStatus = 0x26,
	nAttach = 0x27,
	nMetadata = 0x28,
	nEffect = 0x29,
	nRemoveEffect = 0x2A,
	nExperience = 0x2B,
	nPreChunk = 0x32,
	nMultiBlock = 0x34,
	nBlockChange = 0x35,
	nNoteBlock = 0x36,
	nExplosion = 0x3C,
	nDoorChange = 0x3D,
	nBed = 0x46,
	nWeather = 0x47,
	nOpenWindow = 0x64,
	nCloseWindow = 0x65,
	nWindowClick = 0x66,
	nSetSlot = 0x67,
	nWindowItems = 0x68,
	nProgressBar = 0x69,
	nTransaction = 0x6A,
	nCreativeSetSlot = 0x6B,
	nEnchantItem = 0x6C,
	nUpdateSign = 0x82,
	nPlayerInfo = 0xC9,
	nAbilities = 0xCA,
	nKick = 0xFF,
};

void logf(const char *fmt, ...)
{
	if (g_log == nullptr)
		return;
	char buf[256];
	va_list args;
	va_start(args, fmt);
	std::vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	g_log(buf);
}

void push(ClientboundOut &out, uint8_t id, Writer &w)
{
	out.packets.emplace_back();
	out.packets.back().id = id;
	out.packets.back().payload.swap(w.b);
}

void pushKick(ClientboundOut &out, const std::string &reason)
{
	Writer w;
	w.str125(truncateUtf8(reason, 200), 256);
	push(out, nKick, w);
}

// Long chat lines are split: the native reader caps a message at 119 units.
void pushChat(ClientboundOut &out, const std::string &text)
{
	std::size_t start = 0;
	const std::size_t total = text.size();
	int guard = 0;
	do
	{
		std::string piece = truncateUtf8(text.substr(start), 100);
		if (piece.empty() && start < total)
			break;
		Writer w;
		w.str125(piece, 119);
		push(out, nChat, w);
		start += piece.size();
	} while (start < total && ++guard < 8);
}

int32_t toFixed32(double v)
{
	return static_cast<int32_t>(std::floor(v * 32.0));
}

int64_t toFixed4096(double v)
{
	return static_cast<int64_t>(std::floor(v * 4096.0));
}

int32_t floorDiv128(int64_t v)
{
	return static_cast<int32_t>(v >= 0 ? v / 128 : -((-v + 127) / 128));
}

int stageOf(Session &s)
{
	return s.stage.load(std::memory_order_acquire);
}

// -- entity metadata (1.12 -> native watcher) ---------------------------------

struct MetaHarvest
{
	bool gotSlot = false;
	int16_t id = -1;
	uint8_t count = 0;
	int16_t damage = 0;
};

// Native watcher entry writers (key = type << 5 | index).
void wByte(Writer &w, int index, int v)
{
	w.u8(static_cast<uint8_t>((0 << 5) | index));
	w.u8(static_cast<uint8_t>(v));
}
void wShort(Writer &w, int index, int v)
{
	w.u8(static_cast<uint8_t>((1 << 5) | index));
	w.i16(static_cast<int16_t>(v));
}
void wInt(Writer &w, int index, int32_t v)
{
	w.u8(static_cast<uint8_t>((2 << 5) | index));
	w.i32(v);
}

bool isAgeable(EntityKind k)
{
	return k == EntityKind::Ageable || k == EntityKind::Pig || k == EntityKind::Sheep ||
	       k == EntityKind::Wolf || k == EntityKind::Ocelot || k == EntityKind::Villager;
}

bool convertMetadata(Cursor &c, EntityKind kind, Writer &w, MetaHarvest &harvest)
{
	int written = 0;
	for (;;)
	{
		const uint8_t index = c.u8();
		if (!c.ok)
			return false;
		if (index == 0xFF)
			break;
		const uint32_t type = c.varint();
		int32_t vInt = 0;
		float vFloat = 0.0f;
		bool numeric = true;
		switch (type)
		{
		case 0: vInt = c.u8(); break;            // byte
		case 1: vInt = c.svarint(); break;       // varint
		case 2: vFloat = c.f32(); vInt = static_cast<int32_t>(vFloat); break;
		case 3: case 4: c.strMc(); numeric = false; break; // string, chat
		case 5: // slot
		{
			const int16_t id = c.i16();
			harvest.id = id;
			if (id >= 0)
			{
				harvest.count = c.u8();
				harvest.damage = c.i16();
				const std::size_t nbt = skipNbt(c.p, c.left);
				if (nbt == 0)
					return false;
				c.skip(nbt);
			}
			numeric = false;
			if ((kind == EntityKind::Item || kind == EntityKind::Potion) && index == 6)
				harvest.gotSlot = true;
			break;
		}
		case 6: vInt = c.u8(); break;            // boolean
		case 7: c.skip(12); numeric = false; break; // rotation
		case 8: c.skip(8); numeric = false; break;  // position
		case 9: if (c.u8()) c.skip(8); numeric = false; break; // optional position
		case 10: vInt = c.svarint(); break;      // direction
		case 11: if (c.u8()) c.skip(16); numeric = false; break; // optional uuid
		case 12: vInt = c.svarint(); break;      // optional block state
		case 13:
		{
			const std::size_t nbt = skipNbt(c.p, c.left);
			if (nbt == 0)
				return false;
			c.skip(nbt);
			numeric = false;
			break;
		}
		default:
			return false;
		}
		if (!c.ok)
			return false;
		if (!numeric || written > 24)
			continue;

		const int before = static_cast<int>(w.b.size());
		if (index == 0 && type == 0)
			wByte(w, 0, vInt & 0x0F); // fire, sneak, (riding), sprint
		else if (index == 1 && type == 1)
			wShort(w, 1, vInt < 0 ? 0 : (vInt > 300 ? 300 : vInt)); // air
		else if (index == 12 && type == 6 && isAgeable(kind))
			wInt(w, 12, vInt ? -24000 : 0); // baby
		else
		{
			switch (kind)
			{
			case EntityKind::Pig:
				if (index == 13 && type == 6) wByte(w, 16, vInt);
				break;
			case EntityKind::Sheep:
				if (index == 13 && type == 0) wByte(w, 16, vInt);
				break;
			case EntityKind::Wolf:
				if (index == 13 && type == 0) wByte(w, 16, vInt & 7);
				else if (index == 15 && type == 2) wInt(w, 18, static_cast<int32_t>(vFloat));
				break;
			case EntityKind::Ocelot:
				if (index == 13 && type == 0) wByte(w, 16, vInt & 7);
				else if (index == 15 && type == 1) wByte(w, 18, vInt & 3);
				break;
			case EntityKind::Creeper:
				if (index == 12 && type == 1) wByte(w, 16, vInt);
				else if (index == 13 && type == 6) wByte(w, 17, vInt);
				break;
			case EntityKind::Slime:
				if (index == 12 && type == 1) wByte(w, 16, vInt);
				break;
			case EntityKind::Ghast:
				if (index == 12 && type == 6) wByte(w, 16, vInt);
				break;
			case EntityKind::Spider:
			case EntityKind::Blaze:
			case EntityKind::IronGolem:
				if (index == 12 && type == 0) wByte(w, 16, vInt);
				break;
			case EntityKind::Enderman:
				if (index == 12 && type == 12)
				{
					uint8_t bid = 0, bmeta = 0;
					if (vInt > 0)
						mapBlock(static_cast<uint32_t>(vInt) >> 4, static_cast<uint32_t>(vInt) & 15, bid, bmeta);
					wByte(w, 16, bid);
					wByte(w, 17, bmeta);
				}
				break;
			case EntityKind::Villager:
				if (index == 13 && type == 1) wInt(w, 16, vInt < 0 || vInt > 4 ? 0 : vInt);
				break;
			case EntityKind::Dragon:
				if (index == 7 && type == 2) wInt(w, 16, static_cast<int32_t>(vFloat));
				break;
			case EntityKind::Boat:
			case EntityKind::Minecart:
				if (index == 6 && type == 1) wInt(w, 17, vInt);
				else if (index == 7 && type == 1) wInt(w, 18, vInt);
				else if (index == 8 && type == 2) wInt(w, 19, static_cast<int32_t>(vFloat));
				break;
			default:
				break;
			}
		}
		if (static_cast<int>(w.b.size()) != before)
			written++;
	}
	w.u8(0x7F);
	return true;
}

struct MobPlan
{
	bool drop = false;
	uint8_t type = 0;
	EntityKind kind = EntityKind::Living;
};

MobPlan planMob(uint32_t t)
{
	MobPlan p;
	if ((t == 64 || t == 65 || t == 66) && g_nativeBlockExists != nullptr && g_nativeBlockExists(158))
	{
		p.type = static_cast<uint8_t>(t); // 1.5.2 engine: wither, bat, witch exist
		return p;
	}
	switch (t)
	{
	case 50: p.type = 50; p.kind = EntityKind::Creeper; break;
	case 51: case 5: case 6: case 64: p.type = 51; break; // skeletons, wither
	case 52: p.type = 52; p.kind = EntityKind::Spider; break;
	case 53: p.type = 53; break;
	case 54: case 23: case 27: p.type = 54; break; // zombie, husk, zombie villager
	case 55: p.type = 55; p.kind = EntityKind::Slime; break;
	case 56: p.type = 56; p.kind = EntityKind::Ghast; break;
	case 57: p.type = 57; break;
	case 58: p.type = 58; p.kind = EntityKind::Enderman; break;
	case 59: p.type = 59; p.kind = EntityKind::Spider; break;
	case 60: case 67: p.type = 60; break; // silverfish, endermite
	case 61: p.type = 61; p.kind = EntityKind::Blaze; break;
	case 62: p.type = 62; p.kind = EntityKind::Slime; break;
	case 63: p.type = 63; p.kind = EntityKind::Dragon; break;
	case 66: case 34: case 36: case 37: p.type = 120; p.kind = EntityKind::Villager; break; // witch, illagers
	case 68: case 4: p.type = 94; break; // guardians -> squid
	case 90: p.type = 90; p.kind = EntityKind::Pig; break;
	case 91: p.type = 91; p.kind = EntityKind::Sheep; break;
	case 92: case 28: case 29: case 31: case 32: case 100: case 103: case 102:
		p.type = 92; p.kind = EntityKind::Ageable; break; // cow, horses, llama, polar bear
	case 93: case 101: case 105: p.type = 93; p.kind = EntityKind::Ageable; break; // chicken, rabbit, parrot
	case 94: p.type = 94; break;
	case 95: p.type = 95; p.kind = EntityKind::Wolf; break;
	case 96: p.type = 96; p.kind = EntityKind::Ageable; break;
	case 97: p.type = 97; break;
	case 98: p.type = 98; p.kind = EntityKind::Ocelot; break;
	case 99: p.type = 99; p.kind = EntityKind::IronGolem; break;
	case 120: p.type = 120; p.kind = EntityKind::Villager; break;
	default: p.drop = true; break; // bat, shulker, vex, armor stand...
	}
	return p;
}

EntityTrack &track(Session &s, int32_t eid, EntityKind kind, double x, double y, double z)
{
	if (s.entities.size() > 4096)
		s.entities.clear();
	EntityTrack &t = s.entities[eid];
	t = EntityTrack();
	t.kind = kind;
	t.x = toFixed4096(x);
	t.y = toFixed4096(y);
	t.z = toFixed4096(z);
	t.nx = toFixed32(x);
	t.ny = toFixed32(y);
	t.nz = toFixed32(z);
	return t;
}

// Relative move in 1/4096 units -> native relative move (1/32 units) or a
// teleport when the native byte cannot hold the step.
void relMove(Session &s, ClientboundOut &out, int32_t eid, int16_t dx, int16_t dy, int16_t dz,
             bool look, int8_t yaw, int8_t pitch)
{
	auto it = s.entities.find(eid);
	if (it == s.entities.end())
	{
		if (look)
		{
			Writer w;
			w.i32(eid);
			w.i8(yaw);
			w.i8(pitch);
			push(out, nLookE, w);
		}
		return;
	}
	EntityTrack &t = it->second;
	t.x += dx;
	t.y += dy;
	t.z += dz;
	if (look)
	{
		t.yaw = yaw;
		t.pitch = pitch;
	}
	const int32_t nx = floorDiv128(t.x), ny = floorDiv128(t.y), nz = floorDiv128(t.z);
	const int32_t ddx = nx - t.nx, ddy = ny - t.ny, ddz = nz - t.nz;
	Writer w;
	w.i32(eid);
	if (ddx < -128 || ddx > 127 || ddy < -128 || ddy > 127 || ddz < -128 || ddz > 127)
	{
		w.i32(nx);
		w.i32(ny);
		w.i32(nz);
		w.i8(t.yaw);
		w.i8(t.pitch);
		push(out, nTeleport, w);
	}
	else if (ddx == 0 && ddy == 0 && ddz == 0)
	{
		if (!look)
			return;
		w.i8(yaw);
		w.i8(pitch);
		push(out, nLookE, w);
	}
	else
	{
		w.i8(static_cast<int8_t>(ddx));
		w.i8(static_cast<int8_t>(ddy));
		w.i8(static_cast<int8_t>(ddz));
		if (look)
		{
			w.i8(yaw);
			w.i8(pitch);
		}
		push(out, look ? nRelMoveLook : nRelMove, w);
	}
	t.nx = nx;
	t.ny = ny;
	t.nz = nz;
}

std::string readUuid(Cursor &c)
{
	std::string uuid(16, '\0');
	for (int i = 0; i < 16; i++)
		uuid[static_cast<std::size_t>(i)] = static_cast<char>(c.u8());
	return uuid;
}

// Sign block entity NBT -> native Packet130 (signs carry their text as JSON).
bool signFromNbt(const uint8_t *d, std::size_t len, ClientboundOut &out)
{
	if (len < 3 || d[0] != 10)
		return false;
	std::size_t off = 3 + static_cast<std::size_t>((d[1] << 8) | d[2]);
	int32_t x = 0, y = 0, z = 0;
	std::string lines[4];
	bool isSign = false;
	for (;;)
	{
		if (off >= len)
			return false;
		const uint8_t tag = d[off++];
		if (tag == 0)
			break;
		if (off + 2 > len)
			return false;
		const std::size_t nameLen = static_cast<std::size_t>((d[off] << 8) | d[off + 1]);
		off += 2;
		if (off + nameLen > len)
			return false;
		const std::string name(reinterpret_cast<const char *>(d + off), nameLen);
		off += nameLen;
		if (tag == 8)
		{
			if (off + 2 > len)
				return false;
			const std::size_t sl = static_cast<std::size_t>((d[off] << 8) | d[off + 1]);
			if (off + 2 + sl > len)
				return false;
			const std::string value(reinterpret_cast<const char *>(d + off + 2), sl);
			off += 2 + sl;
			if (name == "id")
				isSign = value == "minecraft:sign" || value == "Sign";
			else if (name.size() == 5 && name.compare(0, 4, "Text") == 0 && name[4] >= '1' && name[4] <= '4')
				lines[name[4] - '1'] = value;
		}
		else if (tag == 3)
		{
			if (off + 4 > len)
				return false;
			const int32_t v = static_cast<int32_t>((static_cast<uint32_t>(d[off]) << 24) |
			                                       (static_cast<uint32_t>(d[off + 1]) << 16) |
			                                       (static_cast<uint32_t>(d[off + 2]) << 8) | d[off + 3]);
			off += 4;
			if (name == "x") x = v;
			else if (name == "y") y = v;
			else if (name == "z") z = v;
		}
		else
		{
			const std::size_t used = skipNbtPayload(d + off, len - off, tag);
			if (used == 0 && tag != 0)
				return false;
			off += used;
		}
	}
	if (!isSign || y < 0 || y > 255)
		return false;
	Writer w;
	w.i32(x);
	w.i16(static_cast<int16_t>(y));
	w.i32(z);
	for (int i = 0; i < 4; i++)
		w.str125(truncateUtf8(chatToText(lines[i]), 15), 15);
	push(out, nUpdateSign, w);
	return true;
}

// 1.12 world effect id -> native aux SFX (0 = drop).
int32_t mapEffect(int32_t id)
{
	switch (id)
	{
	case 1000: return 1000;
	case 1001: return 1001;
	case 1002: return 1002;
	case 1005: case 1006: case 1007: case 1008: case 1011: case 1012: case 1013: case 1014:
		return 1003;
	case 1009: return 1004;
	case 1010: return 1005;
	case 1015: return 1007;
	case 1016: case 1018: return 1008;
	case 1019: return 1010;
	case 1020: return 1011;
	case 1021: return 1012;
	case 2000: return 2000;
	case 2001: return 2001;
	case 2002: return 2002;
	case 2003: return 2003;
	case 2004: return 2004;
	default: return 0;
	}
}

} // namespace

std::vector<uint8_t> encodeHandshake(const std::string &host, uint16_t port, uint32_t nextState)
{
	Writer body;
	body.varint(0x00);
	body.varint(kTargetProtocol);
	body.strMc(host);
	body.u16(port);
	body.varint(nextState);
	Writer frame;
	frame.varint(static_cast<uint32_t>(body.b.size()));
	frame.raw(body.b);
	return frame.b;
}

std::vector<uint8_t> encodeLoginStart(const std::string &username)
{
	Writer body;
	body.varint(0x00);
	body.strMc(username);
	Writer frame;
	frame.varint(static_cast<uint32_t>(body.b.size()));
	frame.raw(body.b);
	return frame.b;
}

// -- serverbound -----------------------------------------------------------------

bool translateServerbound(Session &s, uint8_t nativeId, const uint8_t *payload, std::size_t len,
                          std::vector<std::pair<uint32_t, std::vector<uint8_t>>> &out)
{
	if (stageOf(s) != static_cast<int>(Session::Stage::Play))
		return false; // handshake handled by the IO layer; login swallowed
	Cursor c(payload, len);
	Writer w;
	uint32_t id = 0;
	switch (nativeId)
	{
	case nKeepAlive:
	{
		// The native client also sends unsolicited keepalives (id 0, e.g. on
		// the terrain screen); a 1.12 server kicks for any keepalive it did
		// not ask for, so only echoes of a pending challenge go through.
		const int32_t key = c.i32();
		int64_t value = 0;
		bool pending = false;
		{
			std::lock_guard<std::mutex> guard(s.lock);
			for (int i = 0; i < 4; i++)
				if (s.keepAlivePending[i] && static_cast<int32_t>(s.keepAlive[i]) == key)
				{
					value = s.keepAlive[i];
					s.keepAlivePending[i] = false;
					pending = true;
					break;
				}
		}
		if (!pending)
			return false;
		w.i64(value);
		id = sKeepAlive;
		break;
	}
	case nChat:
		w.strMc(truncateUtf8(c.str125(119), 256));
		id = sChat;
		break;
	case nUseEntity:
	{
		c.i32();
		const int32_t target = c.i32();
		const bool attack = c.u8() != 0;
		w.varint(static_cast<uint32_t>(target));
		w.varint(attack ? 1u : 0u);
		if (!attack)
			w.varint(0); // main hand
		id = sUseEntity;
		break;
	}
	case nFlying:
		w.u8(c.u8());
		id = sPlayer;
		break;
	case nPos:
	case nPosLook:
	{
		const double x = c.f64(), y = c.f64();
		c.f64(); // stance
		const double z = c.f64();
		float yaw = 0.0f, pitch = 0.0f;
		if (nativeId == nPosLook)
		{
			yaw = c.f32();
			pitch = c.f32();
		}
		const uint8_t ground = c.u8();
		if (!c.ok)
			return false;
		{
			std::lock_guard<std::mutex> guard(s.lock);
			s.posX = x;
			s.posY = y;
			s.posZ = z;
			if (nativeId == nPosLook)
			{
				s.yaw = yaw;
				s.pitch = pitch;
			}
		}
		w.f64(x);
		w.f64(y);
		w.f64(z);
		if (nativeId == nPosLook)
		{
			w.f32(yaw);
			w.f32(pitch);
		}
		w.u8(ground);
		id = nativeId == nPos ? sPlayerPosition : sPlayerPosLook;
		break;
	}
	case nLook:
	{
		const float yaw = c.f32(), pitch = c.f32();
		const uint8_t ground = c.u8();
		{
			std::lock_guard<std::mutex> guard(s.lock);
			s.yaw = yaw;
			s.pitch = pitch;
		}
		w.f32(yaw);
		w.f32(pitch);
		w.u8(ground);
		id = sPlayerLook;
		break;
	}
	case nDig:
	{
		const uint8_t status = c.u8();
		const int32_t x = c.i32();
		const uint8_t y = c.u8();
		const int32_t z = c.i32();
		const int8_t face = c.i8();
		if (status > 5)
			return false;
		w.varint(status);
		w.i64(encodePosition(x, y, z));
		w.i8(face);
		id = sPlayerDigging;
		break;
	}
	case nPlace:
	{
		const int32_t x = c.i32();
		const uint8_t y = c.u8();
		const int32_t z = c.i32();
		const int8_t dir = c.i8();
		if (!c.ok)
			return false;
		if (dir < 0 || dir > 5)
		{
			w.varint(0); // Use Item, main hand
			id = sUseItem;
			break;
		}
		w.i64(encodePosition(x, y, z));
		w.varint(static_cast<uint32_t>(dir));
		w.varint(0);
		w.f32(0.5f);
		w.f32(0.5f);
		w.f32(0.5f);
		id = sBlockPlacement;
		break;
	}
	case nHeld:
		w.i16(c.i16());
		id = sHeldItemChange;
		break;
	case nAnimation:
	{
		c.i32();
		const int8_t anim = c.i8();
		if (anim != 1)
			return false;
		w.varint(0);
		id = sAnimation;
		break;
	}
	case nEntityAction:
	{
		c.i32();
		const uint8_t action = c.u8();
		if (action < 1 || action > 5)
			return false;
		w.varint(static_cast<uint32_t>(s.entityId.load()));
		w.varint(static_cast<uint32_t>(action - 1));
		w.varint(0);
		id = sEntityAction;
		break;
	}
	case nRespawn:
		w.varint(0);
		id = sClientStatus;
		break;
	case nCloseWindow:
		w.u8(static_cast<uint8_t>(c.i8()));
		id = sCloseWindow;
		break;
	case nWindowClick:
	{
		const int8_t windowId = c.i8();
		const int16_t slot = c.i16();
		const int8_t button = c.i8();
		const int16_t action = c.i16();
		const bool shift = c.u8() != 0;
		Writer stack;
		if (!convertSlotFromNative(c, stack))
			return false;
		w.u8(static_cast<uint8_t>(windowId));
		w.i16(slot);
		w.i8(button);
		w.i16(action);
		w.varint(shift ? 1u : 0u);
		w.raw(stack.b);
		id = sClickWindow;
		break;
	}
	case nTransaction:
		w.u8(static_cast<uint8_t>(c.i8()));
		w.i16(c.i16());
		w.u8(c.u8());
		id = sConfirmTransaction;
		break;
	case nCreativeSetSlot:
	{
		const int16_t slot = c.i16();
		Writer stack;
		if (!convertSlotFromNative(c, stack))
			return false;
		if (slot < -1 || slot > 44)
			return false;
		w.i16(slot);
		w.raw(stack.b);
		id = sCreativeAction;
		break;
	}
	case nEnchantItem:
		w.u8(static_cast<uint8_t>(c.i8()));
		w.u8(static_cast<uint8_t>(c.i8()));
		id = sEnchantItem;
		break;
	case nUpdateSign:
	{
		const int32_t x = c.i32();
		const int16_t y = c.i16();
		const int32_t z = c.i32();
		w.i64(encodePosition(x, y, z));
		for (int i = 0; i < 4; i++)
			w.strMc(c.str125(15));
		id = sUpdateSign;
		break;
	}
	case nAbilities:
	{
		const bool invulnerable = c.u8() != 0;
		const bool flying = c.u8() != 0;
		const bool allowFlying = c.u8() != 0;
		const bool creative = c.u8() != 0;
		w.u8(static_cast<uint8_t>((invulnerable ? 1 : 0) | (flying ? 2 : 0) | (allowFlying ? 4 : 0) |
		                          (creative ? 8 : 0)));
		w.f32(0.05f);
		w.f32(0.1f);
		id = sPlayerAbilities;
		break;
	}
	default:
		return false; // login, quit, plugin channels...
	}
	if (!c.ok)
		return false;
	out.emplace_back(id, std::move(w.b));
	return true;
}

// -- clientbound -----------------------------------------------------------------

void translateClientbound(Session &s, uint32_t id, const uint8_t *payload, std::size_t len,
                          ClientboundOut &out)
{
	Cursor c(payload, len);
	const int stage = stageOf(s);
	if (stage == static_cast<int>(Session::Stage::Login))
	{
		switch (id)
		{
		case 0x00: // disconnect
			pushKick(out, chatToText(c.strMc()));
			return;
		case 0x01: // encryption request
			pushKick(out, "This server is in online mode (premium accounts only); "
			              "only offline-mode 1.12.2 servers are supported.");
			return;
		case 0x02: // login success
		{
			c.strMc();
			const std::string name = c.strMc();
			if (!name.empty())
				s.username = name;
			Writer w;
			w.str125("-", 64);
			push(out, nHandshake, w);
			s.stage.store(static_cast<int>(Session::Stage::Play), std::memory_order_release);
			return;
		}
		case 0x03: // set compression
			s.compressionThreshold.store(c.svarint(), std::memory_order_release);
			return;
		default:
			return;
		}
	}
	if (stage != static_cast<int>(Session::Stage::Play))
		return;

	Writer w;
	switch (id)
	{
	case cKeepAlive:
	{
		const int64_t v = c.i64();
		if (!c.ok)
			return;
		{
			std::lock_guard<std::mutex> guard(s.lock);
			s.keepAlive[s.keepAliveNext] = v;
			s.keepAlivePending[s.keepAliveNext] = true;
			s.keepAliveNext = (s.keepAliveNext + 1) & 3;
		}
		w.i32(static_cast<int32_t>(v));
		push(out, nKeepAlive, w);
		return;
	}
	case cJoinGame:
	{
		const int32_t eid = c.i32();
		const uint8_t mode = static_cast<uint8_t>(c.u8() & 7);
		const int32_t dim = c.i32();
		const uint8_t difficulty = c.u8();
		const uint8_t maxPlayers = c.u8();
		const std::string level = c.strMc();
		if (!c.ok)
			return;
		s.entityId.store(eid);
		s.dimension.store(dim);
		s.entities.clear();
		s.passengers.clear();
		s.windowKinds.clear();
		w.i32(eid);
		w.str125(truncateUtf8(s.username, 16), 16);
		w.str125(level == "flat" ? "flat" : "default", 16);
		w.i32(mode == 1 || mode == 3 ? 1 : 0);
		w.i32(dim);
		w.u8(difficulty);
		w.u8(128);
		w.u8(maxPlayers);
		push(out, nLogin, w);
		// The vanilla client announces its brand and settings right after
		// joining; some server plugins expect both.
		Writer brand;
		brand.strMc("MC|Brand");
		brand.strMc("vanilla");
		s.inject(sPluginMessage, brand);
		Writer settings;
		settings.strMc("en_us");
		settings.u8(6);      // view distance
		settings.varint(0);  // chat enabled
		settings.u8(1);      // colours
		settings.u8(0x7F);   // skin parts
		settings.varint(1);  // main hand: right
		s.inject(sClientSettings, settings);
		logf("[mc340] join game eid=%d mode=%d dim=%d", eid, mode, dim);
		return;
	}
	case cChat:
	{
		const std::string json = c.strMc();
		const uint8_t position = c.u8();
		if (!c.ok || position == 2)
			return; // action bar: would flood the chat
		pushChat(out, chatToText(json));
		return;
	}
	case cTimeUpdate:
	{
		c.i64();
		int64_t time = c.i64();
		if (time < 0)
			time = -time;
		w.i64(time);
		push(out, nTime, w);
		return;
	}
	case cSpawnPosition:
	{
		int32_t x, y, z;
		decodePosition(c.i64(), x, y, z);
		{
			// First estimate of where the player is (the chunk window centres
			// on it until the real teleport arrives).
			std::lock_guard<std::mutex> guard(s.lock);
			if (s.posX == 0.0 && s.posZ == 0.0)
			{
				s.posX = x + 0.5;
				s.posZ = z + 0.5;
			}
		}
		w.i32(x);
		w.i32(y);
		w.i32(z);
		push(out, nSpawnPos, w);
		return;
	}
	case cUpdateHealth:
	{
		const float health = c.f32();
		int32_t food = c.svarint();
		const float saturation = c.f32();
		if (!c.ok)
			return;
		int32_t hp = static_cast<int32_t>(health + 0.5f);
		if (health > 0.0f && hp < 1)
			hp = 1;
		if (hp < 0)
			hp = 0;
		if (hp > 20)
			hp = 20;
		food = food < 0 ? 0 : (food > 20 ? 20 : food);
		w.i16(static_cast<int16_t>(hp));
		w.i16(static_cast<int16_t>(food));
		w.f32(saturation);
		push(out, nHealth, w);
		return;
	}
	case cRespawn:
	{
		const int32_t dim = c.i32();
		const uint8_t difficulty = c.u8();
		const uint8_t mode = static_cast<uint8_t>(c.u8() & 7);
		const std::string level = c.strMc();
		if (!c.ok)
			return;
		s.dimension.store(dim);
		s.entities.clear();
		s.passengers.clear();
		w.i32(dim);
		w.u8(difficulty);
		w.u8(mode == 1 || mode == 3 ? 1 : 0);
		w.i16(256);
		w.str125(level == "flat" ? "flat" : "default", 16);
		push(out, nRespawn, w);
		return;
	}
	case cPlayerPosLook:
	{
		double x = c.f64(), y = c.f64(), z = c.f64();
		float yaw = c.f32(), pitch = c.f32();
		const uint8_t flags = c.u8();
		const uint32_t teleportId = c.varint();
		if (!c.ok)
			return;
		{
			std::lock_guard<std::mutex> guard(s.lock);
			if (flags & 0x01) x += s.posX;
			if (flags & 0x02) y += s.posY;
			if (flags & 0x04) z += s.posZ;
			if (flags & 0x08) yaw += s.yaw;
			if (flags & 0x10) pitch += s.pitch;
			s.posX = x;
			s.posY = y;
			s.posZ = z;
			s.yaw = yaw;
			s.pitch = pitch;
		}
		Writer confirm;
		confirm.varint(teleportId);
		s.inject(sTeleportConfirm, confirm);
		w.f64(x);
		w.f64(y);
		w.f64(y + 1.62);
		w.f64(z);
		w.f32(yaw);
		w.f32(pitch);
		w.u8(0);
		push(out, nPosLook, w);
		return;
	}
	case cPlayerAbilities:
	{
		const uint8_t flags = c.u8();
		w.u8(flags & 1 ? 1 : 0);
		w.u8(flags & 2 ? 1 : 0);
		w.u8(flags & 4 ? 1 : 0);
		w.u8(flags & 8 ? 1 : 0);
		push(out, nAbilities, w);
		return;
	}
	case cChangeGameState:
	{
		const uint8_t reason = c.u8();
		const float value = c.f32();
		if (!c.ok)
			return;
		int8_t state = -1, mode = 0;
		if (reason == 0)
			state = 0;
		else if (reason == 1)
			state = 2; // 1.12 "end raining" = native 2
		else if (reason == 2)
			state = 1; // 1.12 "begin raining" = native 1
		else if (reason == 3)
		{
			state = 3;
			const int m = static_cast<int>(value);
			mode = static_cast<int8_t>(m == 1 || m == 3 ? 1 : 0);
		}
		if (state < 0)
			return;
		w.i8(state);
		w.i8(mode);
		push(out, nBed, w);
		return;
	}
	case cEntityEquipment:
	{
		const int32_t eid = c.svarint();
		const uint32_t slot = c.varint();
		Writer stack;
		Cursor peek = c;
		int16_t itemId = peek.i16();
		int16_t damage = 0;
		if (itemId >= 0)
		{
			peek.u8();
			damage = peek.i16();
			itemId = mapItem(itemId, damage);
		}
		if (!c.ok || slot == 1 || slot > 5)
			return;
		w.i32(eid);
		w.i16(static_cast<int16_t>(slot == 0 ? 0 : slot - 1));
		w.i16(itemId);
		w.i16(damage);
		push(out, nPlayerInventory, w);
		return;
	}
	case cSpawnPlayer:
	{
		const int32_t eid = c.svarint();
		const std::string uuid = readUuid(c);
		const double x = c.f64(), y = c.f64(), z = c.f64();
		const int8_t yaw = c.i8(), pitch = c.i8();
		if (!c.ok)
			return;
		EntityTrack &t = track(s, eid, EntityKind::Player, x, y, z);
		t.yaw = yaw;
		t.pitch = pitch;
		std::string name = "Player";
		auto found = s.playerNames.find(uuid);
		if (found != s.playerNames.end() && !found->second.empty())
			name = found->second;
		w.i32(eid);
		w.str125(truncateUtf8(name, 16), 16);
		w.i32(t.nx);
		w.i32(t.ny);
		w.i32(t.nz);
		w.i8(yaw);
		w.i8(pitch);
		w.i16(0);
		push(out, nNamedEntitySpawn, w);
		MetaHarvest harvest;
		Writer meta;
		meta.i32(eid);
		if (convertMetadata(c, EntityKind::Player, meta, harvest) && meta.b.size() > 5)
			push(out, nMetadata, meta);
		return;
	}
	case cSpawnObject:
	{
		const int32_t eid = c.svarint();
		readUuid(c);
		const uint8_t type = c.u8();
		const double x = c.f64(), y = c.f64(), z = c.f64();
		c.i8();
		c.i8();
		const int32_t data = c.i32();
		const int16_t vx = c.i16(), vy = c.i16(), vz = c.i16();
		if (!c.ok)
			return;
		uint8_t nativeType = 0;
		EntityKind kind = EntityKind::Unknown;
		switch (type)
		{
		case 1: nativeType = 1; kind = EntityKind::Boat; break;
		case 2: case 73:
		{
			EntityTrack &t = track(s, eid, type == 2 ? EntityKind::Item : EntityKind::Potion, x, y, z);
			t.pending = true;
			t.potion = type == 73;
			t.vx = vx;
			t.vy = vy;
			t.vz = vz;
			return;
		}
		case 10: nativeType = data == 1 ? 11 : (data == 2 ? 12 : 10); kind = EntityKind::Minecart; break;
		case 50: case 51: case 60: case 61: case 62: case 63: case 64: case 65: case 72: case 75: case 90:
			nativeType = type;
			break;
		case 91: nativeType = 60; break; // spectral arrow
		case 66: nativeType = 64; break; // wither skull
		case 93: nativeType = 63; break; // dragon fireball
		case 70:
		{
			const int32_t block = data & 0xFFF;
			nativeType = block == 13 ? 71 : (block == 122 ? 74 : 70);
			break;
		}
		default:
			return; // item frame, armor stand, firework, leash knot...
		}
		EntityTrack &t = track(s, eid, kind, x, y, z);
		w.i32(eid);
		w.u8(nativeType);
		w.i32(t.nx);
		w.i32(t.ny);
		w.i32(t.nz);
		const bool projectile = nativeType >= 60 && nativeType != 70 && nativeType != 71 && nativeType != 74;
		int32_t thrower = 0;
		if (projectile)
			thrower = (nativeType == 60 || nativeType == 90) && data > 0 ? data : 1;
		w.i32(thrower);
		if (thrower > 0)
		{
			w.i16(vx);
			w.i16(vy);
			w.i16(vz);
		}
		push(out, nVehicleSpawn, w);
		return;
	}
	case cSpawnGlobal:
	{
		const int32_t eid = c.svarint();
		const uint8_t type = c.u8();
		const double x = c.f64(), y = c.f64(), z = c.f64();
		if (!c.ok || type != 1)
			return;
		w.i32(eid);
		w.u8(1);
		w.i32(toFixed32(x));
		w.i32(toFixed32(y));
		w.i32(toFixed32(z));
		push(out, nWeather, w);
		return;
	}
	case cSpawnMob:
	{
		const int32_t eid = c.svarint();
		readUuid(c);
		const uint32_t type = c.varint();
		const double x = c.f64(), y = c.f64(), z = c.f64();
		const int8_t yaw = c.i8(), pitch = c.i8(), headPitch = c.i8();
		c.i16();
		c.i16();
		c.i16();
		if (!c.ok)
			return;
		const MobPlan plan = planMob(type);
		if (plan.drop)
			return;
		EntityTrack &t = track(s, eid, plan.kind, x, y, z);
		t.yaw = yaw;
		t.pitch = pitch;
		w.i32(eid);
		w.u8(plan.type);
		w.i32(t.nx);
		w.i32(t.ny);
		w.i32(t.nz);
		w.i8(yaw);
		w.i8(pitch);
		w.i8(headPitch);
		MetaHarvest harvest;
		if (!convertMetadata(c, plan.kind, w, harvest))
			return;
		push(out, nMobSpawn, w);
		return;
	}
	case cSpawnPainting:
	{
		const int32_t eid = c.svarint();
		readUuid(c);
		const std::string title = c.strMc();
		int32_t x, y, z;
		decodePosition(c.i64(), x, y, z);
		const uint8_t dir = c.u8();
		if (!c.ok)
			return;
		static const char *const kArts[] = {"Kebab", "Aztec", "Alban", "Aztec2", "Bomb", "Plant", "Wasteland",
		                                    "Pool", "Courbet", "Sea", "Sunset", "Creebet", "Wanderer", "Graham",
		                                    "Match", "Bust", "Stage", "Void", "SkullAndRoses", "Fighters",
		                                    "Pointer", "Pigscene", "BurningSkull", "Skeleton", "DonkeyKong"};
		std::string art = "Kebab";
		for (const char *a : kArts)
			if (title == a && std::strcmp(a, "DonkeyKong") != 0)
				art = a;
		// 1.12 facing (0 south, 1 west, 2 north, 3 east) -> native direction
		// (0 north, 1 west, 2 south, 3 east); native x/y/z is the wall block.
		int32_t nativeDir = 0;
		switch (dir & 3)
		{
		case 0: nativeDir = 2; z -= 1; break;
		case 1: nativeDir = 1; x += 1; break;
		case 2: nativeDir = 0; z += 1; break;
		default: nativeDir = 3; x -= 1; break;
		}
		track(s, eid, EntityKind::Unknown, x, y, z);
		w.i32(eid);
		w.str125(art, 13);
		w.i32(x);
		w.i32(y);
		w.i32(z);
		w.i32(nativeDir);
		push(out, nPainting, w);
		return;
	}
	case cSpawnExpOrb:
	{
		const int32_t eid = c.svarint();
		const double x = c.f64(), y = c.f64(), z = c.f64();
		const int16_t count = c.i16();
		if (!c.ok)
			return;
		EntityTrack &t = track(s, eid, EntityKind::Unknown, x, y, z);
		w.i32(eid);
		w.i32(t.nx);
		w.i32(t.ny);
		w.i32(t.nz);
		w.i16(count);
		push(out, nExpOrb, w);
		return;
	}
	case cEntityVelocity:
	{
		const int32_t eid = c.svarint();
		const int16_t vx = c.i16(), vy = c.i16(), vz = c.i16();
		if (!c.ok)
			return;
		w.i32(eid);
		w.i16(vx);
		w.i16(vy);
		w.i16(vz);
		push(out, nVelocity, w);
		return;
	}
	case cDestroyEntities:
	{
		const uint32_t count = c.varint();
		if (!c.ok || count > 4096)
			return;
		for (uint32_t i = 0; i < count; i++)
		{
			const int32_t eid = c.svarint();
			if (!c.ok)
				return;
			auto it = s.entities.find(eid);
			const bool pending = it != s.entities.end() && it->second.pending;
			if (it != s.entities.end())
				s.entities.erase(it);
			s.passengers.erase(eid);
			if (pending)
				continue; // never reached the native world
			Writer d;
			d.i32(eid);
			push(out, nDestroy, d);
		}
		return;
	}
	case cEntityRelMove:
	{
		const int32_t eid = c.svarint();
		const int16_t dx = c.i16(), dy = c.i16(), dz = c.i16();
		if (!c.ok)
			return;
		relMove(s, out, eid, dx, dy, dz, false, 0, 0);
		return;
	}
	case cEntityLookRelMove:
	{
		const int32_t eid = c.svarint();
		const int16_t dx = c.i16(), dy = c.i16(), dz = c.i16();
		const int8_t yaw = c.i8(), pitch = c.i8();
		if (!c.ok)
			return;
		relMove(s, out, eid, dx, dy, dz, true, yaw, pitch);
		return;
	}
	case cEntityLook:
	{
		const int32_t eid = c.svarint();
		const int8_t yaw = c.i8(), pitch = c.i8();
		if (!c.ok)
			return;
		auto it = s.entities.find(eid);
		if (it != s.entities.end())
		{
			it->second.yaw = yaw;
			it->second.pitch = pitch;
		}
		w.i32(eid);
		w.i8(yaw);
		w.i8(pitch);
		push(out, nLookE, w);
		return;
	}
	case cEntityTeleport:
	{
		const int32_t eid = c.svarint();
		const double x = c.f64(), y = c.f64(), z = c.f64();
		const int8_t yaw = c.i8(), pitch = c.i8();
		if (!c.ok)
			return;
		auto it = s.entities.find(eid);
		if (it == s.entities.end())
			return;
		EntityTrack &t = it->second;
		t.x = toFixed4096(x);
		t.y = toFixed4096(y);
		t.z = toFixed4096(z);
		t.nx = toFixed32(x);
		t.ny = toFixed32(y);
		t.nz = toFixed32(z);
		t.yaw = yaw;
		t.pitch = pitch;
		if (t.pending)
			return;
		w.i32(eid);
		w.i32(t.nx);
		w.i32(t.ny);
		w.i32(t.nz);
		w.i8(yaw);
		w.i8(pitch);
		push(out, nTeleport, w);
		return;
	}
	case cEntityHeadLook:
	{
		const int32_t eid = c.svarint();
		const int8_t yaw = c.i8();
		if (!c.ok)
			return;
		w.i32(eid);
		w.i8(yaw);
		push(out, nHeadRotation, w);
		return;
	}
	case cEntityStatus:
	{
		const int32_t eid = c.i32();
		const int8_t status = c.i8();
		// 24..28: op permission level 0..4, only ever sent about the local player.
		if (c.ok && status >= 24 && status <= 28)
			s_localOpLevel.store(status - 24, std::memory_order_relaxed);
		if (!c.ok || status < 0 || status > 11)
			return;
		w.i32(eid);
		w.i8(status);
		push(out, nStatus, w);
		return;
	}
	case cSetPassengers:
	{
		const int32_t vehicle = c.svarint();
		const uint32_t count = c.varint();
		if (!c.ok || count > 16)
			return;
		std::vector<int32_t> now;
		for (uint32_t i = 0; i < count; i++)
			now.push_back(c.svarint());
		if (!c.ok)
			return;
		std::vector<int32_t> &before = s.passengers[vehicle];
		for (int32_t p : before)
		{
			bool still = false;
			for (int32_t q : now)
				still = still || q == p;
			if (!still)
			{
				Writer d;
				d.i32(p);
				d.i32(-1);
				push(out, nAttach, d);
			}
		}
		if (!now.empty())
		{
			Writer a;
			a.i32(now[0]); // the native engine has one rider per vehicle
			a.i32(vehicle);
			push(out, nAttach, a);
		}
		before = now;
		if (before.empty())
			s.passengers.erase(vehicle);
		return;
	}
	case cEntityMetadata:
	{
		const int32_t eid = c.svarint();
		auto it = s.entities.find(eid);
		const EntityKind kind = it != s.entities.end() ? it->second.kind : EntityKind::Unknown;
		MetaHarvest harvest;
		w.i32(eid);
		if (!convertMetadata(c, kind, w, harvest))
			return;
		if (it != s.entities.end() && it->second.pending)
		{
			if (!harvest.gotSlot || harvest.id < 0)
				return;
			EntityTrack &t = it->second;
			Writer spawn;
			int16_t damage = harvest.damage;
			if (!t.potion)
			{
				const int16_t nid = mapItem(harvest.id, damage);
				if (nid < 0)
					return;
				spawn.i32(eid);
				spawn.i16(nid);
				spawn.u8(harvest.count == 0 ? 1 : harvest.count);
				spawn.i16(damage);
				spawn.i32(t.nx);
				spawn.i32(t.ny);
				spawn.i32(t.nz);
				spawn.i8(static_cast<int8_t>(t.vx / 64));
				spawn.i8(static_cast<int8_t>(t.vy / 64));
				spawn.i8(static_cast<int8_t>(t.vz / 64));
				push(out, nPickupSpawn, spawn);
			}
			else
			{
				const int32_t potion = damage > 0 ? damage : 16384;
				spawn.i32(eid);
				spawn.u8(73);
				spawn.i32(t.nx);
				spawn.i32(t.ny);
				spawn.i32(t.nz);
				spawn.i32(potion);
				spawn.i16(t.vx);
				spawn.i16(t.vy);
				spawn.i16(t.vz);
				push(out, nVehicleSpawn, spawn);
			}
			t.pending = false;
			return;
		}
		if (w.b.size() > 5)
			push(out, nMetadata, w);
		return;
	}
	case cEntityEffect:
	{
		const int32_t eid = c.svarint();
		const int8_t effect = c.i8();
		const int8_t amp = c.i8();
		const int32_t duration = c.svarint();
		if (!c.ok || effect < 1 || effect > 19)
			return;
		w.i32(eid);
		w.i8(effect);
		w.i8(amp);
		w.i16(static_cast<int16_t>(duration > 32767 || duration < 0 ? 32767 : duration));
		push(out, nEffect, w);
		return;
	}
	case cRemoveEntityEffect:
	{
		const int32_t eid = c.svarint();
		const int8_t effect = c.i8();
		if (!c.ok || effect < 1 || effect > 19)
			return;
		w.i32(eid);
		w.i8(effect);
		push(out, nRemoveEffect, w);
		return;
	}
	case cSetExperience:
	{
		const float bar = c.f32();
		const int32_t level = c.svarint();
		const int32_t total = c.svarint();
		if (!c.ok)
			return;
		w.f32(bar);
		w.i16(static_cast<int16_t>(level < 0 ? 0 : (level > 32767 ? 32767 : level)));
		w.i16(static_cast<int16_t>(total < 0 ? 0 : (total > 32767 ? 32767 : total)));
		push(out, nExperience, w);
		return;
	}
	case cAnimation:
	{
		const int32_t eid = c.svarint();
		const uint8_t anim = c.u8();
		if (!c.ok)
			return;
		static const int8_t kMap[] = {1, 2, 3, 1, 6, 7};
		if (anim > 5)
			return;
		w.i32(eid);
		w.i8(kMap[anim]);
		push(out, nAnimation, w);
		return;
	}
	case cCollectItem:
	{
		const int32_t item = c.svarint();
		const int32_t collector = c.svarint();
		if (!c.ok)
			return;
		w.i32(item);
		w.i32(collector);
		push(out, nCollect, w);
		return;
	}
	case cUseBed:
	{
		const int32_t eid = c.svarint();
		int32_t x, y, z;
		decodePosition(c.i64(), x, y, z);
		if (!c.ok)
			return;
		w.i32(eid);
		w.i8(0);
		w.i32(x);
		w.i8(static_cast<int8_t>(y));
		w.i32(z);
		push(out, nSleep, w);
		return;
	}
	case cOpenWindow:
	{
		const uint8_t windowId = c.u8();
		const std::string type = c.strMc();
		const std::string title = c.strMc();
		const uint8_t slots = c.u8();
		if (!c.ok)
			return;
		int nativeType = -1;
		if (type == "minecraft:chest" || type == "minecraft:container" || type == "minecraft:shulker_box")
			nativeType = 0;
		else if (type == "minecraft:crafting_table")
			nativeType = 1;
		else if (type == "minecraft:furnace")
			nativeType = 2;
		else if (type == "minecraft:dispenser" || type == "minecraft:dropper")
			nativeType = 3;
		if (nativeType < 0)
		{
			// Enchanting/brewing changed their slot layout, villagers, anvils,
			// hoppers, beacons and horses do not exist natively: close it on
			// the server so its inventory state stays in step.
			Writer close;
			close.u8(windowId);
			s.inject(sCloseWindow, close);
			pushChat(out, "\xC2\xA7" "7(this container is not supported on this version)");
			return;
		}
		s.windowKinds[windowId] = nativeType;
		std::string text = chatToText(title);
		w.u8(windowId);
		w.u8(static_cast<uint8_t>(nativeType));
		w.str125(truncateUtf8(text, 32), 32);
		w.u8(slots);
		push(out, nOpenWindow, w);
		return;
	}
	case cCloseWindow:
	{
		const uint8_t windowId = c.u8();
		s.windowKinds.erase(windowId);
		w.i8(static_cast<int8_t>(windowId));
		push(out, nCloseWindow, w);
		return;
	}
	case cSetSlot:
	{
		const int8_t windowId = c.i8();
		const int16_t slot = c.i16();
		if (windowId > 0 && s.windowKinds.find(windowId) == s.windowKinds.end())
			return;
		if (windowId == 0 && slot >= 45)
			return; // off-hand slot
		Writer stack;
		if (!convertSlotToNative(c, stack))
			return;
		w.i8(windowId);
		w.i16(slot);
		w.raw(stack.b);
		push(out, nSetSlot, w);
		return;
	}
	case cWindowItems:
	{
		const uint8_t windowId = c.u8();
		int16_t count = c.i16();
		if (!c.ok || count < 0)
			return;
		if (windowId > 0 && s.windowKinds.find(windowId) == s.windowKinds.end())
			return;
		const int16_t keep = windowId == 0 && count > 45 ? 45 : count;
		w.i8(static_cast<int8_t>(windowId));
		w.i16(keep);
		for (int16_t i = 0; i < count; i++)
		{
			Writer stack;
			if (!convertSlotToNative(c, stack))
				return;
			if (i < keep)
				w.raw(stack.b);
		}
		push(out, nWindowItems, w);
		return;
	}
	case cWindowProperty:
	{
		const uint8_t windowId = c.u8();
		const int16_t property = c.i16();
		const int16_t value = c.i16();
		if (!c.ok || s.windowKinds.find(windowId) == s.windowKinds.end())
			return;
		// Furnace: 1.12 0 burn time, 1 total burn, 2 cook progress, 3 total
		// cook; native 0 cook progress, 1 burn time, 2 total burn.
		int16_t nativeProp = property;
		int16_t nativeValue = value;
		if (s.windowKinds[windowId] == 2)
		{
			if (property == 2)
				nativeProp = 0;
			else if (property == 0)
				nativeProp = 1;
			else if (property == 1)
				nativeProp = 2;
			else
				return;
		}
		w.i8(static_cast<int8_t>(windowId));
		w.i16(nativeProp);
		w.i16(nativeValue);
		push(out, nProgressBar, w);
		return;
	}
	case cConfirmTransaction:
	{
		w.i8(c.i8());
		w.i16(c.i16());
		w.u8(c.u8());
		if (!c.ok)
			return;
		push(out, nTransaction, w);
		return;
	}
	case cChunkData:
	{
		const int32_t cx = c.i32(), cz = c.i32();
		const bool groundUp = c.u8() != 0;
		const uint32_t mask = c.varint();
		const uint32_t size = c.varint();
		if (!c.ok || size > c.left)
			return;
		const uint8_t *data = c.p;
		c.skip(size);
		const bool skylight = s.dimension.load() == 0;
		out.chunkAt.push_back(out.packets.size());
		out.chunks.emplace_back();
		ConvertedChunk &chunk = out.chunks.back();
		chunk.x = cx;
		chunk.z = cz;
		if (!convertChunkColumn(data, size, mask, groundUp, skylight, chunk))
		{
			logf("[mc340] bad chunk %d,%d mask=%x size=%u", cx, cz, mask, size);
			out.chunks.pop_back();
			out.chunkAt.pop_back();
			return;
		}
		const uint32_t blockEntities = c.varint();
		for (uint32_t i = 0; c.ok && i < blockEntities && i < 1024; i++)
		{
			const std::size_t nbt = skipNbt(c.p, c.left);
			if (nbt == 0)
				break;
			signFromNbt(c.p, nbt, out);
			c.skip(nbt);
		}
		return;
	}
	case cUnloadChunk:
	{
		const int32_t cx = c.i32(), cz = c.i32();
		if (!c.ok)
			return;
		w.i32(cx);
		w.i32(cz);
		w.u8(0);
		push(out, nPreChunk, w);
		return;
	}
	case cBlockChange:
	{
		int32_t x, y, z;
		decodePosition(c.i64(), x, y, z);
		const uint32_t state = c.varint();
		if (!c.ok || y < 0 || y > 255)
			return;
		uint8_t bid, bmeta;
		mapBlock(state >> 4, state & 15, bid, bmeta);
		w.i32(x);
		w.u8(static_cast<uint8_t>(y));
		w.i32(z);
		w.u8(bid);
		w.u8(bmeta);
		push(out, nBlockChange, w);
		return;
	}
	case cMultiBlockChange:
	{
		const int32_t cx = c.i32(), cz = c.i32();
		const uint32_t count = c.varint();
		if (!c.ok || count == 0 || count > 65535)
			return;
		w.i32(cx);
		w.i32(cz);
		w.i16(static_cast<int16_t>(count));
		w.i32(static_cast<int32_t>(count * 4));
		for (uint32_t i = 0; i < count; i++)
		{
			const uint8_t xz = c.u8();
			const uint8_t y = c.u8();
			const uint32_t state = c.varint();
			if (!c.ok)
				return;
			uint8_t bid, bmeta;
			mapBlock(state >> 4, state & 15, bid, bmeta);
			w.u16(static_cast<uint16_t>((xz << 8) | y)); // x << 12 | z << 8 | y
			w.u16(static_cast<uint16_t>((bid << 4) | bmeta));
		}
		push(out, nMultiBlock, w);
		return;
	}
	case cBlockAction:
	{
		int32_t x, y, z;
		decodePosition(c.i64(), x, y, z);
		const uint8_t action = c.u8();
		const uint8_t param = c.u8();
		const uint32_t block = c.varint();
		if (!c.ok || (block != 25 && block != 29 && block != 33))
			return;
		w.i32(x);
		w.i16(static_cast<int16_t>(y));
		w.i32(z);
		w.u8(action);
		w.u8(param);
		push(out, nNoteBlock, w);
		return;
	}
	case cUpdateBlockEntity:
	{
		int32_t x, y, z;
		decodePosition(c.i64(), x, y, z);
		const uint8_t action = c.u8();
		if (!c.ok || action != 9)
			return;
		const std::size_t nbt = skipNbt(c.p, c.left);
		if (nbt > 1)
			signFromNbt(c.p, nbt, out);
		return;
	}
	case cEffect:
	{
		const int32_t effect = c.i32();
		int32_t x, y, z;
		decodePosition(c.i64(), x, y, z);
		int32_t data = c.i32();
		if (!c.ok || y < -128 || y > 127)
			return;
		const int32_t nativeEffect = mapEffect(effect);
		if (nativeEffect == 0)
			return;
		if (effect == 2001)
		{
			uint8_t bid, bmeta;
			mapBlock(static_cast<uint32_t>(data) & 0xFFF, (static_cast<uint32_t>(data) >> 12) & 15, bid, bmeta);
			data = bid | (bmeta << 12);
		}
		w.i32(nativeEffect);
		w.i32(x);
		w.i8(static_cast<int8_t>(y));
		w.i32(z);
		w.i32(data);
		push(out, nDoorChange, w);
		return;
	}
	case cExplosion:
	{
		const float ex = c.f32(), ey = c.f32(), ez = c.f32(), radius = c.f32();
		const int32_t count = c.i32();
		if (!c.ok || count < 0 || count > 4096 || c.left < static_cast<std::size_t>(count) * 3)
			return;
		w.f64(ex);
		w.f64(ey);
		w.f64(ez);
		w.f32(radius);
		w.i32(count);
		w.raw(c.p, static_cast<std::size_t>(count) * 3);
		push(out, nExplosion, w);
		return;
	}
	case cPlayerListItem:
	{
		const uint32_t action = c.varint();
		const uint32_t count = c.varint();
		if (!c.ok || count > 1024)
			return;
		for (uint32_t i = 0; i < count; i++)
		{
			const std::string uuid = readUuid(c);
			if (action == 0)
			{
				const std::string name = c.strMc();
				const uint32_t props = c.varint();
				for (uint32_t p = 0; p < props && c.ok; p++)
				{
					c.strMc();
					c.strMc();
					if (c.u8())
						c.strMc();
				}
				c.varint();
				const int32_t ping = c.svarint();
				if (c.u8())
					c.strMc();
				if (!c.ok)
					return;
				if (s.playerNames.find(uuid) != s.playerNames.end())
					continue;
				if (s.playerNames.size() < 512)
					s.playerNames[uuid] = name;
				Writer e;
				e.str125(truncateUtf8(name, 16), 16);
				e.u8(1);
				e.i16(static_cast<int16_t>(ping < 0 ? 0 : (ping > 32767 ? 32767 : ping)));
				push(out, nPlayerInfo, e);
			}
			else if (action == 1 || action == 2)
				c.varint();
			else if (action == 3)
			{
				if (c.u8())
					c.strMc();
			}
			else if (action == 4)
			{
				auto found = s.playerNames.find(uuid);
				if (found == s.playerNames.end())
					continue;
				Writer e;
				e.str125(truncateUtf8(found->second, 16), 16);
				e.u8(0);
				e.i16(0);
				push(out, nPlayerInfo, e);
				s.playerNames.erase(found);
			}
			else
				return;
			if (!c.ok)
				return;
		}
		return;
	}
	case cResourcePackSend:
	{
		Writer status;
		status.varint(1); // declined
		s.inject(sResourcePackStatus, status);
		return;
	}
	case cDisconnect:
		pushKick(out, chatToText(c.strMc()));
		return;
	default:
		return; // titles, boss bars, scoreboards, sounds, particles, advancements...
	}
}

int localOpLevel()
{
	return s_localOpLevel.load(std::memory_order_relaxed);
}

void resetLocalOpLevel()
{
	s_localOpLevel.store(-1, std::memory_order_relaxed);
}

} // namespace translator340
