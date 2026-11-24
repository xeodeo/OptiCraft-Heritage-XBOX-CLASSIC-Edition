// ProtocolTranslator340 core: wire primitives, chat JSON, block/item/biome
// mapping, NBT, slots and paletted chunk decoding. The packet tables live in
// ProtocolTranslator340Packets.cpp. See the header for the design.

#include "ProtocolTranslator340.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace translator340
{

void (*g_log)(const char *text) = nullptr;
bool (*g_nativeItemExists)(int16_t id) = nullptr;
bool (*g_nativeBlockExists)(int id) = nullptr;
bool (*g_nativeItemWantsNbt)(int16_t id) = nullptr;
bool (*g_gzipCompress)(const uint8_t *in, std::size_t len, std::vector<uint8_t> &out) = nullptr;
bool (*g_gzipDecompress)(const uint8_t *in, std::size_t len, std::vector<uint8_t> &out) = nullptr;

namespace
{

void encodeUtf8(std::string &out, uint32_t cp)
{
	if (cp < 0x80)
		out.push_back(static_cast<char>(cp));
	else if (cp < 0x800)
	{
		out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	}
	else if (cp < 0x10000)
	{
		out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	}
	else
	{
		out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
		out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
	}
}

std::size_t utf8PointLen(uint8_t lead)
{
	if (lead < 0x80)
		return 1;
	if ((lead & 0xE0) == 0xC0)
		return 2;
	if ((lead & 0xF0) == 0xE0)
		return 3;
	if ((lead & 0xF8) == 0xF0)
		return 4;
	return 0;
}

uint32_t decodeUtf8Point(const uint8_t *p, std::size_t len)
{
	if (len == 1)
		return p[0];
	if (len == 2)
		return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
	if (len == 3)
		return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
	return ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
}

} // namespace

// -- Cursor -----------------------------------------------------------------

uint8_t Cursor::u8()
{
	if (left < 1)
	{
		ok = false;
		return 0;
	}
	const uint8_t v = *p++;
	--left;
	return v;
}

int8_t Cursor::i8()
{
	return static_cast<int8_t>(u8());
}

uint16_t Cursor::u16()
{
	if (left < 2)
	{
		ok = false;
		left = 0;
		return 0;
	}
	const uint16_t v = static_cast<uint16_t>((p[0] << 8) | p[1]);
	p += 2;
	left -= 2;
	return v;
}

int16_t Cursor::i16()
{
	return static_cast<int16_t>(u16());
}

int32_t Cursor::i32()
{
	if (left < 4)
	{
		ok = false;
		left = 0;
		return 0;
	}
	const uint32_t v = (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
	                   (static_cast<uint32_t>(p[2]) << 8) | p[3];
	p += 4;
	left -= 4;
	return static_cast<int32_t>(v);
}

int64_t Cursor::i64()
{
	if (left < 8)
	{
		ok = false;
		left = 0;
		return 0;
	}
	uint64_t v = 0;
	for (int i = 0; i < 8; i++)
		v = (v << 8) | p[i];
	p += 8;
	left -= 8;
	return static_cast<int64_t>(v);
}

float Cursor::f32()
{
	const int32_t bits = i32();
	float v = 0.0f;
	std::memcpy(&v, &bits, 4);
	return v;
}

double Cursor::f64()
{
	const int64_t bits = i64();
	double v = 0.0;
	std::memcpy(&v, &bits, 8);
	return v;
}

uint32_t Cursor::varint()
{
	uint32_t value = 0;
	for (int i = 0; i < 5; i++)
	{
		const uint8_t b = u8();
		if (!ok)
			return 0;
		value |= static_cast<uint32_t>(b & 0x7F) << (7 * i);
		if (!(b & 0x80))
			return value;
	}
	ok = false;
	return 0;
}

std::string Cursor::str125(std::size_t maxChars)
{
	const int16_t n = i16();
	if (!ok || n < 0 || static_cast<std::size_t>(n) > maxChars || left < static_cast<std::size_t>(n) * 2)
	{
		ok = false;
		return std::string();
	}
	std::string out;
	for (int i = 0; i < n; i++)
	{
		uint32_t cp = static_cast<uint32_t>((p[0] << 8) | p[1]);
		p += 2;
		left -= 2;
		if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < n)
		{
			const uint32_t lo = static_cast<uint32_t>((p[0] << 8) | p[1]);
			if (lo >= 0xDC00 && lo <= 0xDFFF)
			{
				p += 2;
				left -= 2;
				i++;
				cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
			}
		}
		encodeUtf8(out, cp);
	}
	return out;
}

std::string Cursor::strMc()
{
	const uint32_t n = varint();
	if (!ok || n > left)
	{
		ok = false;
		return std::string();
	}
	std::string s(reinterpret_cast<const char *>(p), n);
	p += n;
	left -= n;
	return s;
}

void Cursor::skip(std::size_t n)
{
	if (left < n)
	{
		ok = false;
		left = 0;
		return;
	}
	p += n;
	left -= n;
}

// -- Writer -----------------------------------------------------------------

void Writer::u16(uint16_t v)
{
	b.push_back(static_cast<uint8_t>(v >> 8));
	b.push_back(static_cast<uint8_t>(v & 0xFF));
}

void Writer::i32(int32_t v)
{
	const uint32_t u = static_cast<uint32_t>(v);
	b.push_back(static_cast<uint8_t>(u >> 24));
	b.push_back(static_cast<uint8_t>(u >> 16));
	b.push_back(static_cast<uint8_t>(u >> 8));
	b.push_back(static_cast<uint8_t>(u));
}

void Writer::i64(int64_t v)
{
	const uint64_t u = static_cast<uint64_t>(v);
	for (int i = 7; i >= 0; i--)
		b.push_back(static_cast<uint8_t>(u >> (8 * i)));
}

void Writer::f32(float v)
{
	int32_t bits = 0;
	std::memcpy(&bits, &v, 4);
	i32(bits);
}

void Writer::f64(double v)
{
	int64_t bits = 0;
	std::memcpy(&bits, &v, 8);
	i64(bits);
}

void Writer::varint(uint32_t v)
{
	for (;;)
	{
		const uint8_t chunk = static_cast<uint8_t>(v & 0x7F);
		v >>= 7;
		if (v)
			b.push_back(static_cast<uint8_t>(chunk | 0x80));
		else
		{
			b.push_back(chunk);
			return;
		}
	}
}

void Writer::str125(const std::string &utf8, std::size_t maxChars)
{
	std::vector<uint16_t> units;
	const uint8_t *p = reinterpret_cast<const uint8_t *>(utf8.data());
	std::size_t left = utf8.size();
	while (left > 0)
	{
		const std::size_t len = utf8PointLen(*p);
		if (len == 0 || len > left)
			break;
		uint32_t cp = decodeUtf8Point(p, len);
		p += len;
		left -= len;
		if (cp < 0x10000)
		{
			if (units.size() + 1 > maxChars)
				break;
			units.push_back(static_cast<uint16_t>(cp));
		}
		else
		{
			if (units.size() + 2 > maxChars)
				break;
			cp -= 0x10000u;
			units.push_back(static_cast<uint16_t>(0xD800 + (cp >> 10)));
			units.push_back(static_cast<uint16_t>(0xDC00 + (cp & 0x3FF)));
		}
	}
	i16(static_cast<int16_t>(units.size()));
	for (uint16_t u : units)
		u16(u);
}

void Writer::strMc(const std::string &utf8)
{
	varint(static_cast<uint32_t>(utf8.size()));
	raw(reinterpret_cast<const uint8_t *>(utf8.data()), utf8.size());
}

void Session::inject(uint32_t id, const Writer &w)
{
	std::lock_guard<std::mutex> guard(lock);
	if (injected.size() < 64)
		injected.emplace_back(id, w.b);
}

int64_t encodePosition(int32_t x, int32_t y, int32_t z)
{
	const uint64_t ux = static_cast<uint32_t>(x) & 0x3FFFFFFu;
	const uint64_t uy = static_cast<uint32_t>(y) & 0xFFFu;
	const uint64_t uz = static_cast<uint32_t>(z) & 0x3FFFFFFu;
	return static_cast<int64_t>((ux << 38) | (uy << 26) | uz);
}

void decodePosition(int64_t v, int32_t &x, int32_t &y, int32_t &z)
{
	const uint64_t u = static_cast<uint64_t>(v);
	int32_t sx = static_cast<int32_t>((u >> 38) & 0x3FFFFFFu);
	int32_t sy = static_cast<int32_t>((u >> 26) & 0xFFFu);
	int32_t sz = static_cast<int32_t>(u & 0x3FFFFFFu);
	if (sx >= (1 << 25))
		sx -= (1 << 26);
	if (sy >= (1 << 11))
		sy -= (1 << 12);
	if (sz >= (1 << 25))
		sz -= (1 << 26);
	x = sx;
	y = sy;
	z = sz;
}

std::string truncateUtf8(const std::string &text, std::size_t maxCodePoints)
{
	const uint8_t *p = reinterpret_cast<const uint8_t *>(text.data());
	std::size_t left = text.size(), count = 0, bytes = 0;
	while (left > 0 && count < maxCodePoints)
	{
		const std::size_t len = utf8PointLen(*p);
		if (len == 0 || len > left)
			break;
		p += len;
		left -= len;
		bytes += len;
		count++;
	}
	return text.substr(0, bytes);
}

// -- chat JSON ----------------------------------------------------------------
//
// A tiny JSON reader, enough for chat components (objects, arrays, strings,
// numbers, booleans). Malformed input returns the raw text.

namespace
{

struct JNode
{
	enum Type : uint8_t
	{
		Null,
		Bool,
		Number,
		String,
		Array,
		Object
	} type = Null;
	bool boolean = false;
	std::string text;
	std::vector<JNode> items;
	std::vector<std::string> keys; // Object: parallel to items

	const JNode *member(const char *key) const
	{
		for (std::size_t i = 0; i < keys.size(); i++)
			if (keys[i] == key)
				return &items[i];
		return nullptr;
	}
};

struct JParser
{
	const char *p;
	const char *end;
	int depth = 0;

	void ws()
	{
		while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
			p++;
	}

	bool str(std::string &out)
	{
		if (p >= end || *p != '"')
			return false;
		p++;
		while (p < end && *p != '"')
		{
			if (*p == '\\')
			{
				p++;
				if (p >= end)
					return false;
				const char e = *p++;
				switch (e)
				{
				case 'n': out.push_back('\n'); break;
				case 't': out.push_back(' '); break;
				case 'r': break;
				case 'b': break;
				case 'f': break;
				case 'u':
				{
					if (end - p < 4)
						return false;
					uint32_t cp = 0;
					for (int i = 0; i < 4; i++)
					{
						const char h = *p++;
						cp <<= 4;
						if (h >= '0' && h <= '9') cp |= static_cast<uint32_t>(h - '0');
						else if (h >= 'a' && h <= 'f') cp |= static_cast<uint32_t>(h - 'a' + 10);
						else if (h >= 'A' && h <= 'F') cp |= static_cast<uint32_t>(h - 'A' + 10);
						else return false;
					}
					if (cp >= 0xD800 && cp <= 0xDBFF && end - p >= 6 && p[0] == '\\' && p[1] == 'u')
					{
						uint32_t lo = 0;
						bool good = true;
						for (int i = 2; i < 6; i++)
						{
							const char h = p[i];
							lo <<= 4;
							if (h >= '0' && h <= '9') lo |= static_cast<uint32_t>(h - '0');
							else if (h >= 'a' && h <= 'f') lo |= static_cast<uint32_t>(h - 'a' + 10);
							else if (h >= 'A' && h <= 'F') lo |= static_cast<uint32_t>(h - 'A' + 10);
							else good = false;
						}
						if (good && lo >= 0xDC00 && lo <= 0xDFFF)
						{
							p += 6;
							cp = 0x10000u + ((cp - 0xD800u) << 10) + (lo - 0xDC00u);
						}
					}
					encodeUtf8(out, cp);
					break;
				}
				default: out.push_back(e); break;
				}
			}
			else
				out.push_back(*p++);
		}
		if (p >= end)
			return false;
		p++;
		return true;
	}

	bool value(JNode &n)
	{
		if (++depth > 32)
			return false;
		ws();
		if (p >= end)
			return false;
		bool ok = true;
		if (*p == '{')
		{
			n.type = JNode::Object;
			p++;
			ws();
			if (p < end && *p == '}')
				p++;
			else
				for (;;)
				{
					ws();
					std::string key;
					if (!str(key))
						return false;
					ws();
					if (p >= end || *p != ':')
						return false;
					p++;
					n.keys.push_back(key);
					n.items.emplace_back();
					if (!value(n.items.back()))
						return false;
					ws();
					if (p < end && *p == ',')
					{
						p++;
						continue;
					}
					if (p < end && *p == '}')
					{
						p++;
						break;
					}
					return false;
				}
		}
		else if (*p == '[')
		{
			n.type = JNode::Array;
			p++;
			ws();
			if (p < end && *p == ']')
				p++;
			else
				for (;;)
				{
					n.items.emplace_back();
					if (!value(n.items.back()))
						return false;
					ws();
					if (p < end && *p == ',')
					{
						p++;
						continue;
					}
					if (p < end && *p == ']')
					{
						p++;
						break;
					}
					return false;
				}
		}
		else if (*p == '"')
		{
			n.type = JNode::String;
			ok = str(n.text);
		}
		else if (end - p >= 4 && std::strncmp(p, "true", 4) == 0)
		{
			n.type = JNode::Bool;
			n.boolean = true;
			p += 4;
		}
		else if (end - p >= 5 && std::strncmp(p, "false", 5) == 0)
		{
			n.type = JNode::Bool;
			p += 5;
		}
		else if (end - p >= 4 && std::strncmp(p, "null", 4) == 0)
		{
			p += 4;
		}
		else
		{
			n.type = JNode::Number;
			while (p < end && (*p == '-' || *p == '+' || *p == '.' || *p == 'e' || *p == 'E' ||
			                   (*p >= '0' && *p <= '9')))
				n.text.push_back(*p++);
			ok = !n.text.empty();
		}
		depth--;
		return ok;
	}
};

char legacyColorCode(const std::string &color)
{
	static const char *const kNames[] = {"black", "dark_blue", "dark_green", "dark_aqua",
	                                      "dark_red", "dark_purple", "gold", "gray",
	                                      "dark_gray", "blue", "green", "aqua",
	                                      "red", "light_purple", "yellow", "white"};
	for (int i = 0; i < 16; i++)
		if (color == kNames[i])
			return "0123456789abcdef"[i];
	return '\0';
}

void appendCode(std::string &out, char code)
{
	out += "\xC2\xA7";
	out.push_back(code);
}

void chatWalk(const JNode &node, std::string &out, char inherited);

std::string chatFlatten(const JNode &node, char inherited)
{
	std::string s;
	chatWalk(node, s, inherited);
	return s;
}

// The handful of translation keys a 1.12 server sends as "translate"
// components; anything else shows its arguments.
const char *translationPattern(const std::string &key)
{
	struct Entry
	{
		const char *key;
		const char *pattern;
	};
	static const Entry kEntries[] = {
	    {"chat.type.text", "<%s> %s"},
	    {"chat.type.announcement", "[%s] %s"},
	    {"chat.type.emote", "* %s %s"},
	    {"chat.type.admin", "[%s: %s]"},
	    {"multiplayer.player.joined", "%s joined the game"},
	    {"multiplayer.player.joined.renamed", "%s (formerly known as %s) joined the game"},
	    {"multiplayer.player.left", "%s left the game"},
	    {"commands.message.display.incoming", "%s whispers to you: %s"},
	    {"commands.message.display.outgoing", "You whisper to %s: %s"},
	    {"chat.type.achievement", "%s has just earned the achievement %s"},
	    {"chat.type.advancement.task", "%s has made the advancement %s"},
	    {"chat.type.advancement.goal", "%s has reached the goal %s"},
	    {"chat.type.advancement.challenge", "%s has completed the challenge %s"},
	    {"death.attack.player", "%s was slain by %s"},
	    {"death.attack.mob", "%s was slain by %s"},
	    {"death.attack.arrow", "%s was shot by %s"},
	    {"death.attack.explosion.player", "%s was blown up by %s"},
	    {"death.attack.inFire", "%s went up in flames"},
	    {"death.attack.onFire", "%s burned to death"},
	    {"death.attack.lava", "%s tried to swim in lava"},
	    {"death.attack.drown", "%s drowned"},
	    {"death.attack.starve", "%s starved to death"},
	    {"death.attack.cactus", "%s was pricked to death"},
	    {"death.attack.generic", "%s died"},
	    {"death.attack.explosion", "%s blew up"},
	    {"death.attack.magic", "%s was killed by magic"},
	    {"death.attack.outOfWorld", "%s fell out of the world"},
	    {"death.attack.inWall", "%s suffocated in a wall"},
	    {"death.fell.accident.generic", "%s fell from a high place"},
	    {"death.attack.fall", "%s hit the ground too hard"},
	    {"commands.generic.unknownCommand", "Unknown command. Try /help for a list of commands"},
	    {"commands.generic.permission", "You do not have permission to use this command"},
	    {"container.chest", "Chest"},
	    {"container.chestDouble", "Large Chest"},
	    {"container.crafting", "Crafting"},
	    {"container.furnace", "Furnace"},
	    {"container.dispenser", "Dispenser"},
	    {"container.dropper", "Dropper"},
	    {"container.enderchest", "Ender Chest"},
	    {"container.shulkerBox", "Shulker Box"},
	    {"container.enchant", "Enchant"},
	    {"container.brewing", "Brewing Stand"},
	    {"multiplayer.disconnect.server_shutdown", "Server closed"},
	    {"multiplayer.disconnect.kicked", "Kicked by an operator"},
	    {"multiplayer.disconnect.outdated_client", "Outdated client! Please use %s"},
	    {"disconnect.timeout", "Timed out"},
	    {"disconnect.spam", "Kicked for spamming"},
	};
	for (const Entry &e : kEntries)
		if (key == e.key)
			return e.pattern;
	return nullptr;
}

void chatWalk(const JNode &node, std::string &out, char inherited)
{
	if (node.type == JNode::String || node.type == JNode::Number)
	{
		out += node.text;
		return;
	}
	if (node.type == JNode::Array)
	{
		for (const JNode &child : node.items)
			chatWalk(child, out, inherited);
		return;
	}
	if (node.type != JNode::Object)
		return;
	char color = inherited;
	if (const JNode *c = node.member("color"))
	{
		const char code = legacyColorCode(c->text);
		if (code != '\0')
			color = code;
	}
	if (color != inherited)
		appendCode(out, color);
	if (const JNode *b = node.member("bold"))
		if (b->boolean)
			appendCode(out, 'l');
	if (const JNode *i = node.member("italic"))
		if (i->boolean)
			appendCode(out, 'o');
	if (const JNode *text = node.member("text"))
		out += text->text;
	if (const JNode *tr = node.member("translate"))
	{
		std::vector<std::string> args;
		if (const JNode *with = node.member("with"))
			for (const JNode &arg : with->items)
				args.push_back(chatFlatten(arg, color));
		const char *pattern = translationPattern(tr->text);
		if (pattern == nullptr)
		{
			if (args.empty())
				out += tr->text;
			for (std::size_t i = 0; i < args.size(); i++)
			{
				if (i)
					out += ' ';
				out += args[i];
			}
		}
		else
		{
			std::size_t next = 0;
			for (const char *q = pattern; *q; q++)
			{
				if (q[0] == '%' && q[1] == 's')
				{
					if (next < args.size())
						out += args[next];
					next++;
					q++;
				}
				else if (q[0] == '%' && q[1] >= '1' && q[1] <= '9' && q[2] == '$' && q[3] == 's')
				{
					const std::size_t idx = static_cast<std::size_t>(q[1] - '1');
					if (idx < args.size())
						out += args[idx];
					q += 3;
				}
				else
					out.push_back(*q);
			}
		}
	}
	if (const JNode *extra = node.member("extra"))
		for (const JNode &child : extra->items)
			chatWalk(child, out, color);
	if (color != inherited || node.member("bold") || node.member("italic"))
		appendCode(out, inherited == '\0' ? 'r' : inherited);
}

} // namespace

std::string chatToText(const std::string &json)
{
	JParser parser{json.data(), json.data() + json.size()};
	JNode root;
	if (json.empty() || !parser.value(root))
		return json;
	std::string out;
	chatWalk(root, out, 'f');
	// Drop a dangling trailing colour reset.
	while (out.size() >= 3 && static_cast<uint8_t>(out[out.size() - 3]) == 0xC2 &&
	       static_cast<uint8_t>(out[out.size() - 2]) == 0xA7)
		out.resize(out.size() - 3);
	return out;
}

bool statusToNative(const std::string &statusJson, std::string &motdOut, int &onlineOut, int &maxOut,
                    int *protocolOut)
{
	JParser parser{statusJson.data(), statusJson.data() + statusJson.size()};
	JNode root;
	if (!parser.value(root) || root.type != JNode::Object)
		return false;
	motdOut.clear();
	if (const JNode *desc = root.member("description"))
	{
		std::string text;
		chatWalk(*desc, text, 'f');
		for (char ch : text)
			if (ch != '\n' && ch != '\r')
				motdOut.push_back(ch);
	}
	motdOut = truncateUtf8(motdOut, 60);
	onlineOut = 0;
	maxOut = 0;
	if (const JNode *players = root.member("players"))
	{
		if (const JNode *o = players->member("online"))
			onlineOut = std::atoi(o->text.c_str());
		if (const JNode *m = players->member("max"))
			maxOut = std::atoi(m->text.c_str());
	}
	if (protocolOut != nullptr)
	{
		*protocolOut = 0;
		if (const JNode *version = root.member("version"))
			if (const JNode *proto = version->member("protocol"))
				*protocolOut = std::atoi(proto->text.c_str());
	}
	return true;
}

// -- blocks / biomes / items ---------------------------------------------------

void mapBlock(uint32_t id, uint32_t meta, uint8_t &outId, uint8_t &outMeta)
{
	meta &= 0xF;
	uint32_t m = meta;
	uint32_t n = id;
	if (g_nativeBlockExists != nullptr && g_nativeBlockExists(158))
	{
		// 1.5.2 engine: blocks 125..158, top slabs and upside-down stairs exist.
		if ((id >= 125 && id <= 158) || id == 43 || id == 44 || id == 53 || id == 67 || id == 108 ||
		    id == 109 || id == 114)
		{
			outId = static_cast<uint8_t>(id);
			outMeta = static_cast<uint8_t>(meta);
			if (id == 143 && ((meta & 7) == 0 || (meta & 7) == 5))
				outId = outMeta = 0;
			return;
		}
	}
	// 1.2.5 has no top-half slabs and no upside-down stairs. Shown as a bottom slab /
	// normal stair, the client collided half a block lower than the server, so the
	// player stuck to or bounced off them (user 23-11 on the 1.12.2 SMP). Walking ON
	// them is what matters: a top slab becomes the full double slab of its material and
	// an upside-down stair the full block of its material.
	if ((meta & 8) != 0 && (id == 44 || id == 126 || id == 182 || id == 205))
	{
		uint32_t sm = meta & 7;
		if (id == 44)
			sm = sm == 6 ? 4 : (sm == 7 ? 0 : sm);
		else if (id == 126)
			sm = 2;
		else if (id == 182)
			sm = 1;
		else
			sm = 0;
		outId = 43;
		outMeta = static_cast<uint8_t>(sm);
		return;
	}
	if ((meta & 4) != 0)
	{
		uint8_t full = 0, fullMeta = 0;
		switch (id)
		{
		case 53: case 134: case 135: case 136: case 163: case 164: full = 5; break; // wooden -> planks
		case 67: full = 4; break;                 // cobblestone
		case 108: full = 45; break;               // brick
		case 109: case 203: full = 98; break;     // stone brick (purpur -> stone brick)
		case 114: full = 112; break;              // nether brick
		case 128: case 180: full = 24; break;     // sandstone
		case 156: full = 98; break;               // quartz -> stone brick
		default: break;
		}
		if (full != 0)
		{
			outId = full;
			outMeta = fullMeta;
			return;
		}
	}
	switch (id)
	{
	case 1: // granite/diorite/andesite
	case 12: // red sand
	case 19: // wet sponge
	case 90: // portal axis
		m = 0;
		break;
	case 3: // coarse dirt / podzol
		m = 0;
		break;
	case 5: // acacia / dark oak planks
		m = meta == 4 ? 0 : (meta == 5 ? 1 : meta);
		break;
	case 6:
		m = (meta & 7) >= 4 ? (meta & 8) : meta;
		break;
	case 17: // log: type only, no axis
		m = meta & 3;
		break;
	case 18:
		m = meta & 0xF;
		break;
	case 38: // the 1.7 flowers
		m = 0;
		break;
	case 43: // double slab
	case 44: // slab: no top-half slabs in 1.2.5
		m = meta & 7;
		if (m == 6)
			m = 4; // nether brick -> brick
		else if (m == 7)
			m = 0; // quartz -> stone
		break;
	case 53: case 67: case 108: case 109: case 114: // stairs: no upside-down
		m = meta & 3;
		break;
	case 77: // stone button on floor/ceiling
		if ((meta & 7) == 0 || (meta & 7) == 5)
		{
			n = 0;
			m = 0;
		}
		break;
	case 69: // lever on a ceiling
		if ((meta & 7) == 0 || (meta & 7) == 7)
			m = (meta & 8) | 5;
		break;
	case 86: case 91: // pumpkin without a face
		m = meta & 3;
		break;
	case 95: // stained glass (1.2.5 id 95 is the locked chest)
		n = 20;
		m = 0;
		break;
	case 96: // trapdoor: no top half
		m = meta & 7;
		break;
	case 97: // monster egg variants
		m = meta > 2 ? 2 : meta;
		break;
	case 125: n = 43; m = 2; break; // double wooden slab
	case 126: n = 44; m = 2; break; // wooden slab
	case 127: n = 0; m = 0; break;  // cocoa
	case 128: case 134: case 135: case 136: case 163: case 164: case 180: case 203:
		n = 53; m = meta & 3; break; // other stairs -> oak stairs
	case 156: n = 109; m = meta & 3; break; // quartz stairs
	case 129: n = 56; m = 0; break; // emerald ore
	case 130: case 146: n = 54; m = 0; break; // ender / trapped chest
	case 131: case 132: n = 0; m = 0; break; // tripwire
	case 133: n = 57; m = 0; break; // emerald block
	case 137: case 210: case 211: case 255: case 218: n = 1; m = 0; break; // command/structure/observer
	case 138: n = 20; m = 0; break; // beacon
	case 139: n = 85; m = 0; break; // cobblestone wall
	case 140: case 144: case 176: case 177: case 209: case 217: case 166:
		// flower pot, skull, banners, end gateway, structure void, barrier
		n = id == 166 ? 20 : 0;
		m = 0;
		break;
	case 141: case 142: n = 59; m = meta & 7; break; // carrots, potatoes
	case 207: n = 59; m = (meta & 3) * 2 + 1; break; // beetroots
	case 143: n = 77; if ((meta & 7) == 0 || (meta & 7) == 5) { n = 0; m = 0; } break; // wooden button
	case 145: n = 42; m = 0; break; // anvil
	case 147: case 148: n = 70; m = meta ? 1 : 0; break; // weighted plates
	case 149: case 150: n = 93; m = meta & 3; break; // comparator -> repeater
	case 151: case 178: n = 44; m = 0; break; // daylight sensor
	case 152: n = 73; m = 0; break; // redstone block
	case 153: case 213: n = 87; m = 0; break; // quartz ore, magma
	case 154: n = 1; m = 0; break; // hopper
	case 155: n = 80; m = 0; break; // quartz block
	case 157: n = 27; m = meta; break; // activator rail
	case 158: n = 23; m = meta & 7; break; // dropper
	case 159: case 219: case 220: case 221: case 222: case 223: case 224: case 225: case 226: case 227:
	case 228: case 229: case 230: case 231: case 232: case 233: case 234: case 251: case 252:
		n = 35;
		m = (id >= 219 && id <= 234) ? id - 219 : meta; // stained clay, shulker boxes, concrete
		break;
	case 235: case 236: case 237: case 238: case 239: case 240: case 241: case 242:
	case 243: case 244: case 245: case 246: case 247: case 248: case 249: case 250:
		n = 35; m = id - 235; break; // glazed terracotta
	case 160: n = 102; m = 0; break; // stained glass pane
	case 161: n = 18; m = (meta & 0xC) | (meta & 1); break; // acacia/dark oak leaves
	case 162: n = 17; m = (meta & 1); break; // acacia/dark oak log
	case 165: n = 35; m = 5; break; // slime block
	case 167: n = 96; m = meta & 7; break; // iron trapdoor
	case 168: n = 98; m = 1; break; // prismarine
	case 169: n = 89; m = 0; break; // sea lantern
	case 170: n = 35; m = 4; break; // hay bale
	case 171: n = 78; m = 0; break; // carpet -> thin snow layer
	case 172: n = 35; m = 12; break; // hardened clay
	case 173: n = 35; m = 15; break; // coal block
	case 174: case 212: n = 79; m = 0; break; // packed / frosted ice
	case 175: // double plant
		if (meta & 8)
		{
			n = 0; m = 0;
		}
		else if (meta == 2 || meta == 3)
		{
			n = 31; m = meta == 2 ? 1 : 2;
		}
		else
		{
			n = meta == 0 ? 37 : 38; m = 0;
		}
		break;
	case 179: n = 24; m = 0; break; // red sandstone
	case 181: n = 43; m = 1; break;
	case 182: n = 44; m = 1; break;
	case 183: case 184: case 185: case 186: case 187: n = 107; m = meta & 7; break; // fence gates
	case 188: case 189: case 190: case 191: case 192: n = 85; m = 0; break; // fences
	case 193: case 194: case 195: case 196: case 197: n = 64; break; // doors
	case 198: n = 50; m = 5; break; // end rod
	case 199: case 200: case 201: case 202: case 204: n = 35; m = 10; break; // chorus, purpur
	case 205: n = 44; m = 0; break; // purpur slab
	case 206: n = 121; m = 0; break; // end bricks
	case 208: n = 60; m = 0; break; // grass path: same 15/16 height as farmland
	case 214: n = 35; m = 14; break; // nether wart block
	case 215: n = 112; m = 0; break; // red nether brick
	case 216: n = 35; m = 0; break; // bone block
	default:
		if (id > 124)
		{
			n = 1;
			m = 0;
		}
		break;
	}
	outId = static_cast<uint8_t>(n);
	outMeta = static_cast<uint8_t>(m & 0xF);
}

uint8_t mapBiome(uint8_t biome)
{
	if (biome <= 22)
		return biome;
	uint8_t b = biome >= 128 ? static_cast<uint8_t>(biome - 128) : biome;
	if (b <= 22)
		return b;
	switch (b)
	{
	case 23: return 21;           // jungle edge
	case 24: return 0;            // deep ocean
	case 25: case 26: return 16;  // stone / cold beach
	case 27: case 29: return 4;   // birch / roofed forest
	case 28: return 18;
	case 30: case 32: return 5;   // cold / mega taiga
	case 31: case 33: return 19;
	case 34: return 3;            // extreme hills+
	case 35: case 36: return 1;   // savanna
	case 37: case 38: case 39: return 2; // mesa
	default: return 1;
	}
}

int16_t mapItem(int16_t id, int16_t &damage)
{
	if (id < 0)
		return -1;
	if (id < 256)
	{
		uint8_t nid = 0, nmeta = 0;
		mapBlock(static_cast<uint32_t>(id), static_cast<uint32_t>(damage) & 0xF, nid, nmeta);
		if (nid == 0)
			return -1;
		if (nid != id || (damage & 0xF) != nmeta)
			damage = nmeta;
		return nid;
	}
	struct Remap
	{
		int16_t from;
		int16_t to;
	};
	static const Remap kRemap[] = {
	    {386, 340}, {387, 340}, {388, 264}, {391, 260}, {392, 260}, {393, 297}, {394, 260},
	    {396, 322}, {398, 346}, {399, 348}, {400, 357}, {403, 340}, {404, 356}, {405, 336},
	    {406, 348}, {407, 328}, {408, 328}, {411, 365}, {412, 366}, {413, 282}, {415, 334},
	    {420, 287}, {421, 339}, {422, 328}, {423, 363}, {424, 364}, {427, 324}, {428, 324},
	    {429, 324}, {430, 324}, {431, 324}, {432, 260}, {434, 260}, {435, 295}, {436, 282},
	    {437, 374}, {438, 373}, {439, 262}, {440, 262}, {441, 373}, {443, 299}, {444, 333},
	    {445, 333}, {446, 333}, {447, 333}, {448, 333}, {452, 371}, {453, 340}, {2267, 2256},
	};
	if (g_nativeItemExists != nullptr ? g_nativeItemExists(id) : (id <= 385 || id >= 2256))
		return id; // the engine has it (1.5.2 knows 386..408 as well)
	for (const Remap &r : kRemap)
		if (r.from == id)
		{
			if (r.to != 373)
				damage = 0;
			return r.to;
		}
	return -1;
}

// -- NBT skipping ---------------------------------------------------------------

namespace
{

bool nbtPayloadEnd(const uint8_t *d, std::size_t len, std::size_t &off, uint8_t tag, int depth);

int32_t peekI32(const uint8_t *d, std::size_t off)
{
	return static_cast<int32_t>((static_cast<uint32_t>(d[off]) << 24) | (static_cast<uint32_t>(d[off + 1]) << 16) |
	                            (static_cast<uint32_t>(d[off + 2]) << 8) | d[off + 3]);
}

bool nbtCompoundEnd(const uint8_t *d, std::size_t len, std::size_t &off, int depth)
{
	for (;;)
	{
		if (off + 1 > len)
			return false;
		const uint8_t tag = d[off++];
		if (tag == 0)
			return true;
		if (off + 2 > len)
			return false;
		const std::size_t nameLen = static_cast<std::size_t>((d[off] << 8) | d[off + 1]);
		off += 2 + nameLen;
		if (off > len || !nbtPayloadEnd(d, len, off, tag, depth + 1))
			return false;
	}
}

bool nbtPayloadEnd(const uint8_t *d, std::size_t len, std::size_t &off, uint8_t tag, int depth)
{
	if (depth > 64)
		return false;
	std::size_t need = 0;
	switch (tag)
	{
	case 1: need = 1; break;
	case 2: need = 2; break;
	case 3: case 5: need = 4; break;
	case 4: case 6: need = 8; break;
	case 7: case 11: case 12:
	{
		if (off + 4 > len)
			return false;
		const int32_t n = peekI32(d, off);
		if (n < 0)
			return false;
		need = 4 + static_cast<std::size_t>(n) * (tag == 7 ? 1u : (tag == 11 ? 4u : 8u));
		break;
	}
	case 8:
		if (off + 2 > len)
			return false;
		need = 2 + static_cast<std::size_t>((d[off] << 8) | d[off + 1]);
		break;
	case 9:
	{
		if (off + 5 > len)
			return false;
		const uint8_t etag = d[off];
		const int32_t n = peekI32(d, off + 1);
		off += 5;
		if (n < 0)
			return false;
		for (int32_t i = 0; i < n; i++)
			if (!nbtPayloadEnd(d, len, off, etag, depth + 1))
				return false;
		return true;
	}
	case 10:
		return nbtCompoundEnd(d, len, off, depth);
	default:
		return false;
	}
	if (off + need > len)
		return false;
	off += need;
	return true;
}

} // namespace

std::size_t skipNbtPayload(const uint8_t *p, std::size_t len, uint8_t tag)
{
	std::size_t off = 0;
	if (!nbtPayloadEnd(p, len, off, tag, 0))
		return 0;
	return off;
}

std::size_t skipNbt(const uint8_t *p, std::size_t len)
{
	if (len < 1)
		return 0;
	if (p[0] == 0)
		return 1;
	if (p[0] != 10 || len < 3)
		return 0;
	std::size_t off = 3 + static_cast<std::size_t>((p[1] << 8) | p[2]);
	if (off > len || !nbtCompoundEnd(p, len, off, 0))
		return 0;
	return off;
}

// -- slots -------------------------------------------------------------------------

bool convertSlotToNative(Cursor &c, Writer &w)
{
	const int16_t id = c.i16();
	if (!c.ok)
		return false;
	if (id < 0)
	{
		w.i16(-1);
		return true;
	}
	const uint8_t count = c.u8();
	int16_t damage = c.i16();
	if (!c.ok || c.left < 1)
		return false;
	const uint8_t *nbt = c.p;
	std::size_t blobLen = 0;
	if (nbt[0] != 0)
	{
		blobLen = skipNbt(nbt, c.left);
		if (blobLen == 0)
			return false;
		c.skip(blobLen);
	}
	else
		c.skip(1);
	const int16_t nid = mapItem(id, damage);
	if (nid < 0)
	{
		w.i16(-1);
		return true;
	}
	w.i16(nid);
	w.u8(count);
	w.i16(damage);
	if (g_nativeItemWantsNbt == nullptr || g_nativeItemWantsNbt(nid))
	{
		std::vector<uint8_t> gz;
		if (blobLen > 0 && nid == id && g_gzipCompress != nullptr && g_gzipCompress(nbt, blobLen, gz) &&
		    gz.size() <= 32767)
		{
			w.i16(static_cast<int16_t>(gz.size()));
			w.raw(gz.data(), gz.size());
		}
		else
			w.i16(-1);
	}
	return true;
}

bool convertSlotFromNative(Cursor &c, Writer &w)
{
	const int16_t id = c.i16();
	if (!c.ok)
		return false;
	if (id < 0)
	{
		w.i16(-1);
		return true;
	}
	const uint8_t count = c.u8();
	const int16_t damage = c.i16();
	int16_t nbtLen = -1;
	const uint8_t *nbt = nullptr;
	if (g_nativeItemWantsNbt == nullptr || g_nativeItemWantsNbt(id))
	{
		nbtLen = c.i16();
		nbt = c.p;
		if (!c.ok)
			return false;
		if (nbtLen > 0)
			c.skip(static_cast<std::size_t>(nbtLen));
	}
	if (!c.ok)
		return false;
	w.i16(id);
	w.u8(count);
	w.i16(damage);
	std::vector<uint8_t> raw;
	if (nbtLen > 0 && g_gzipDecompress != nullptr &&
	    g_gzipDecompress(nbt, static_cast<std::size_t>(nbtLen), raw))
		w.raw(raw);
	else
		w.u8(0);
	return true;
}

// -- chunks ----------------------------------------------------------------------------
//
// 1.12 section: bitsPerBlock(u8), palette (VarInt count + VarInt states; count
// 0 for the 13-bit global palette), VarInt long count, the packed longs
// (entries may straddle two longs), 2048 bytes block light, 2048 bytes sky
// light when the dimension has a sky. A state is id << 4 | meta. The column
// carries 256 biome bytes after the sections when it is ground-up.

bool convertChunkColumn(const uint8_t *data, std::size_t len, uint32_t mask, bool groundUp,
                        bool skylight, ConvertedChunk &out)
{
	mask &= 0xFFFF;
	int sections = 0;
	for (int s = 0; s < 16; s++)
		if (mask & (1u << s))
			sections++;
	const std::size_t n = static_cast<std::size_t>(sections);
	const std::size_t metaBase = n * 4096;
	const std::size_t blockLightBase = metaBase + n * 2048;
	const std::size_t skyBase = blockLightBase + n * 2048;
	const std::size_t biomeBase = skyBase + n * 2048;
	out.data.assign(biomeBase + (groundUp ? 256u : 0u), 0);
	out.primary = static_cast<uint16_t>(mask);
	out.groundUp = groundUp;

	uint8_t *const base = reinterpret_cast<uint8_t *>(out.data.data());
	Cursor c(data, len);
	uint8_t paletteIds[256];
	uint8_t paletteMetas[256];
	std::vector<uint64_t> longs;
	int k = 0;
	for (int s = 0; s < 16; s++)
	{
		if (!(mask & (1u << s)))
			continue;
		const uint8_t bits = c.u8();
		const uint32_t paletteLen = c.varint();
		if (!c.ok || bits == 0 || bits > 16 || paletteLen > 256)
			return false;
		for (uint32_t i = 0; i < paletteLen; i++)
		{
			const uint32_t state = c.varint();
			mapBlock(state >> 4, state & 0xF, paletteIds[i], paletteMetas[i]);
		}
		const uint32_t longCount = c.varint();
		if (!c.ok || longCount > 4096 || static_cast<std::size_t>(longCount) * 8 > c.left)
			return false;
		longs.resize(longCount);
		for (uint32_t i = 0; i < longCount; i++)
			longs[i] = static_cast<uint64_t>(c.i64());
		if (static_cast<std::size_t>(longCount) * 64 < static_cast<std::size_t>(bits) * 4096)
			return false;

		uint8_t *ids = base + static_cast<std::size_t>(k) * 4096;
		uint8_t *metas = base + metaBase + static_cast<std::size_t>(k) * 2048;
		const uint32_t valueMask = (1u << bits) - 1u;
		const bool direct = paletteLen == 0;
		uint32_t lastState = 0xFFFFFFFFu;
		uint8_t lastId = 0, lastMeta = 0;
		for (uint32_t i = 0; i < 4096; i++)
		{
			const uint32_t bitIndex = i * bits;
			const uint32_t li = bitIndex >> 6;
			const uint32_t off = bitIndex & 63;
			uint64_t v = longs[li] >> off;
			if (off + bits > 64)
				v |= longs[li + 1] << (64 - off);
			const uint32_t value = static_cast<uint32_t>(v) & valueMask;
			uint8_t bid, bmeta;
			if (!direct)
			{
				if (value >= paletteLen)
				{
					bid = 0;
					bmeta = 0;
				}
				else
				{
					bid = paletteIds[value];
					bmeta = paletteMetas[value];
				}
			}
			else
			{
				if (value != lastState)
				{
					mapBlock(value >> 4, value & 0xF, lastId, lastMeta);
					lastState = value;
				}
				bid = lastId;
				bmeta = lastMeta;
			}
			ids[i] = bid;
			if (i & 1)
				metas[i >> 1] |= static_cast<uint8_t>(bmeta << 4);
			else
				metas[i >> 1] = bmeta;
		}
		if (c.left < (skylight ? 4096u : 2048u))
			return false;
		std::memcpy(base + blockLightBase + static_cast<std::size_t>(k) * 2048, c.p, 2048);
		c.skip(2048);
		uint8_t *sky = base + skyBase + static_cast<std::size_t>(k) * 2048;
		if (skylight)
		{
			std::memcpy(sky, c.p, 2048);
			c.skip(2048);
		}
		else
			std::memset(sky, 0xFF, 2048);
		k++;
	}
	if (groundUp)
	{
		if (c.left < 256)
			return false;
		for (int i = 0; i < 256; i++)
			base[biomeBase + static_cast<std::size_t>(i)] = mapBiome(c.p[i]);
		c.skip(256);
	}
	return c.ok;
}

} // namespace translator340
