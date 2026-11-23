// ProtocolTranslator340IO.cpp -- see the header.

#include "ProtocolTranslator340IO.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>

#include <zlib.h>

#include "Block.h"
#include "Item.h"
#include "Packet.h"
#include "Packet51MapChunk.h"
#include "java/JavaNetwork.h"
#include "platform/Log.h"
#include "platform/Diagnostics.h"

namespace
{

bool nativeItemExistsImpl(int16_t id)
{
	return id >= 0 && id < Item::ITEM_LIST_SIZE && Item::itemsList[id] != nullptr;
}

bool nativeBlockExistsImpl(int id)
{
	return id > 0 && id < 256 && Block::blocksList[id] != nullptr;
}

bool nativeItemWantsNbtImpl(int16_t id)
{
	if (id < 0 || id >= Item::ITEM_LIST_SIZE)
		return false;
	Item *item = Item::itemsList[id];
	return item != nullptr && (item->isDamagable() || item->func_46056_k());
}

// GZIP container (what CompressedStreamTools reads for slot NBT).
bool gzipCompressImpl(const uint8_t *in, std::size_t len, std::vector<uint8_t> &out)
{
	out.assign(compressBound(static_cast<uLong>(len)) + 32, 0);
	z_stream zs{};
	if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK)
		return false;
	zs.next_in = const_cast<Bytef *>(in);
	zs.avail_in = static_cast<uInt>(len);
	zs.next_out = out.data();
	zs.avail_out = static_cast<uInt>(out.size());
	const int rc = deflate(&zs, Z_FINISH);
	const uLong produced = zs.total_out;
	deflateEnd(&zs);
	if (rc != Z_STREAM_END)
		return false;
	out.resize(produced);
	return true;
}

bool gzipDecompressImpl(const uint8_t *in, std::size_t len, std::vector<uint8_t> &out)
{
	if (len == 0 || len > 65536)
		return false;
	out.assign(16 * 1024, 0);
	z_stream zs{};
	if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK)
		return false;
	zs.next_in = const_cast<Bytef *>(in);
	zs.avail_in = static_cast<uInt>(len);
	for (;;)
	{
		zs.next_out = out.data() + zs.total_out;
		zs.avail_out = static_cast<uInt>(out.size() - zs.total_out);
		const int rc = inflate(&zs, Z_FINISH);
		if (rc == Z_STREAM_END)
			break;
		if ((rc != Z_OK && rc != Z_BUF_ERROR) || zs.avail_out != 0 || out.size() >= 512 * 1024)
		{
			inflateEnd(&zs);
			return false;
		}
		out.resize(out.size() * 2);
	}
	out.resize(zs.total_out);
	inflateEnd(&zs);
	return true;
}

void logImpl(const char *text)
{
	MC_LOG_INFO("net340", "%s\n", text);
}

struct HookInstaller
{
	HookInstaller()
	{
		translator340::g_nativeItemExists = &nativeItemExistsImpl;
		translator340::g_nativeItemWantsNbt = &nativeItemWantsNbtImpl;
		translator340::g_nativeBlockExists = &nativeBlockExistsImpl;
		translator340::g_gzipCompress = &gzipCompressImpl;
		translator340::g_gzipDecompress = &gzipDecompressImpl;
		translator340::g_log = &logImpl;
	}
} g_hookInstaller;

struct Eof
{
};

constexpr std::size_t kMaxFrameBytes = 2 * 1024 * 1024;
constexpr uint32_t kMaxPlainBytes = 2 * 1024 * 1024;

uint32_t readVarintStream(std::istream &is)
{
	uint32_t value = 0;
	for (int i = 0; i < 5; i++)
	{
		const int b = is.get();
		if (b == std::char_traits<char>::eof())
			throw Eof();
		value |= static_cast<uint32_t>(b & 0x7F) << (7 * i);
		if (!(b & 0x80))
			return value;
	}
	throw std::runtime_error("1.12.2 frame varint too long");
}

std::unique_ptr<Packet> decodeNative(uint8_t id, const std::vector<uint8_t> &payload)
{
	std::string bytes;
	bytes.reserve(payload.size() + 1);
	bytes.push_back(static_cast<char>(id));
	bytes.append(reinterpret_cast<const char *>(payload.data()), payload.size());
	std::istringstream iss(bytes);
	std::unique_ptr<Packet> packet = Packet::readPacket(iss, false);
	if (!packet)
		throw std::runtime_error("1.12.2 translator produced an undecodable packet " + std::to_string(id));
	return packet;
}

} // namespace

Translator340Connection::Translator340Connection(const std::string &host, int port) : host_(host), port_(port)
{
}

std::unique_ptr<Packet> Translator340Connection::readOneTranslated(std::istream &is)
{
	for (;;)
	{
		if (!pending_.empty())
		{
			std::unique_ptr<Packet> packet = std::move(pending_.front());
			pending_.pop_front();
			return packet;
		}
		uint32_t total = 0;
		try
		{
			total = readVarintStream(is);
		}
		catch (const Eof &)
		{
			return nullptr;
		}
		if (total == 0 || total > kMaxFrameBytes)
			throw std::runtime_error("Bad 1.12.2 frame length");
		frame_.resize(total);
		is.read(reinterpret_cast<char *>(frame_.data()), static_cast<std::streamsize>(total));
		if (static_cast<uint32_t>(is.gcount()) != total)
			throw std::runtime_error("Truncated 1.12.2 frame");

		translator340::Cursor c(frame_.data(), frame_.size());
		if (session_.compressionThreshold.load(std::memory_order_acquire) >= 0)
		{
			const uint32_t plainLen = c.varint();
			if (!c.ok || plainLen > kMaxPlainBytes)
				throw std::runtime_error("Bad 1.12.2 compression header");
			if (plainLen != 0)
			{
				plain_.resize(plainLen);
				uLongf actual = plainLen;
				if (uncompress(plain_.data(), &actual, c.p, static_cast<uLong>(c.left)) != Z_OK ||
				    actual != plainLen)
					throw std::runtime_error("Bad 1.12.2 compressed frame");
				c = translator340::Cursor(plain_.data(), plain_.size());
			}
		}
		const uint32_t id = c.varint();
		if (!c.ok)
			throw std::runtime_error("Bad 1.12.2 packet id");

		translator340::ClientboundOut out;
		translator340::translateClientbound(session_, id, c.p, c.left, out);
		std::size_t pi = 0;
		for (std::size_t ci = 0; ci <= out.chunks.size(); ci++)
		{
			const std::size_t upto = ci < out.chunks.size() ? out.chunkAt[ci] : out.packets.size();
			for (; pi < upto; pi++)
				routeNative(out.packets[pi]);
			if (ci == out.chunks.size())
				break;
			routeChunk(out.chunks[ci]);
		}
		updateWindow(false);
		// Keep the reusable buffers from pinning a huge frame forever.
		if (frame_.capacity() > 256 * 1024)
			std::vector<uint8_t>().swap(frame_);
		if (plain_.capacity() > 256 * 1024)
			std::vector<uint8_t>().swap(plain_);
	}
}

// -- chunk window --------------------------------------------------------------
//
// A 1.12.2 server may stream a much larger view distance than a 64 MB console
// can hold as live columns. Every column is kept zlib-compressed here (a few
// KB each); only the ones within radius_ of the player become native
// columns. Columns that fall behind are unloaded natively and re-sent from
// the compressed copy (plus the block changes recorded meanwhile) when the
// player comes back, so the server never has to resend anything.

namespace
{
// Live window radius: 5 (11 x 11 columns) normally; it shrinks while the heap
// runs low (an SMP spawn full of tall builds costs ~100 KB per live column)
// and grows back once there is room again. A live column is unloaded one
// ring past the radius.
constexpr int kMaxRadius = 5;
constexpr int kMinRadius = 2;
constexpr long kShrinkBelowKb = 9 * 1024;
constexpr long kGrowAboveKb = 16 * 1024;
constexpr int kPromotionsPerRead = 8; // spreads re-sends over several frames
constexpr std::size_t kMaxChangesPerColumn = 512;

std::size_t planeOffset(uint16_t mask, int plane, int section)
{
	// Native layout: ids (4096/section), then meta, block light, sky light
	// (2048/section each), then biomes.
	int count = 0, before = 0;
	for (int s = 0; s < 16; s++)
		if (mask & (1u << s))
		{
			if (s < section)
				before++;
			count++;
		}
	const std::size_t n = static_cast<std::size_t>(count);
	if (plane == 0)
		return static_cast<std::size_t>(before) * 4096;
	return n * 4096 + (static_cast<std::size_t>(plane - 1) * n + before) * 2048;
}

std::size_t layoutSize(uint16_t mask, bool biomes)
{
	int count = 0;
	for (int s = 0; s < 16; s++)
		if (mask & (1u << s))
			count++;
	return static_cast<std::size_t>(count) * 10240 + (biomes ? 256u : 0u);
}

bool deflateColumn(const std::vector<int8_t> &raw, std::vector<uint8_t> &out)
{
	uLongf bound = compressBound(static_cast<uLong>(raw.size()));
	out.resize(bound);
	if (compress2(out.data(), &bound, reinterpret_cast<const Bytef *>(raw.data()), static_cast<uLong>(raw.size()),
	              Z_BEST_SPEED) != Z_OK)
		return false;
	out.resize(bound);
	out.shrink_to_fit();
	return true;
}

bool inflateColumn(const std::vector<uint8_t> &z, std::size_t size, std::vector<int8_t> &out)
{
	out.resize(size);
	uLongf actual = static_cast<uLongf>(size);
	return uncompress(reinterpret_cast<Bytef *>(out.data()), &actual, z.data(), static_cast<uLong>(z.size())) ==
	           Z_OK &&
	       actual == size;
}

int64_t columnKey(int32_t x, int32_t z)
{
	return (static_cast<int64_t>(x) << 32) | static_cast<uint32_t>(z);
}

int32_t readBE32(const uint8_t *p)
{
	return static_cast<int32_t>((static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
	                            (static_cast<uint32_t>(p[2]) << 8) | p[3]);
}
} // namespace

bool Translator340Connection::inWindow(int32_t x, int32_t z, int radius) const
{
	return x >= centerX_ - radius && x <= centerX_ + radius && z >= centerZ_ - radius && z <= centerZ_ + radius;
}

void Translator340Connection::emitColumn(int32_t x, int32_t z, Column &col)
{
	std::vector<int8_t> raw;
	if (!inflateColumn(col.z, layoutSize(col.primary, true), raw))
		return;
	translator340::Writer pre;
	pre.i32(x);
	pre.i32(z);
	pre.u8(1);
	pending_.push_back(decodeNative(0x32, pre.b));
	std::unique_ptr<Packet51MapChunk> map(new Packet51MapChunk());
	map->setDecoded(x, z, true, col.primary, std::move(raw));
	pending_.push_back(std::move(map));
	for (uint32_t change : col.changes)
	{
		// packed: x(4) z(4) y(8) id(8) meta(4)
		translator340::Writer w;
		w.i32(x * 16 + static_cast<int32_t>((change >> 28) & 15));
		w.u8(static_cast<uint8_t>((change >> 16) & 0xFF));
		w.i32(z * 16 + static_cast<int32_t>((change >> 24) & 15));
		w.u8(static_cast<uint8_t>((change >> 4) & 0xFF));
		w.u8(static_cast<uint8_t>(change & 15));
		pending_.push_back(decodeNative(0x35, w.b));
	}
	for (const std::vector<uint8_t> &sign : col.signs)
		pending_.push_back(decodeNative(0x82, sign));
	col.live = true;
}

void Translator340Connection::recordChange(int32_t bx, int32_t by, int32_t bz, uint8_t id, uint8_t meta)
{
	const int32_t cx = bx >> 4, cz = bz >> 4;
	auto it = columns_.find(columnKey(cx, cz));
	if (it == columns_.end())
		return;
	Column &col = it->second;
	const uint32_t packed = (static_cast<uint32_t>(bx & 15) << 28) | (static_cast<uint32_t>(bz & 15) << 24) |
	                        (static_cast<uint32_t>(by & 0xFF) << 16) | (static_cast<uint32_t>(id) << 4) |
	                        (meta & 15u);
	if (col.changes.size() < kMaxChangesPerColumn)
	{
		col.changes.push_back(packed);
		return;
	}
	// Too many: fold them into the compressed copy.
	std::vector<int8_t> raw;
	if (!inflateColumn(col.z, layoutSize(col.primary, true), raw))
		return;
	col.changes.push_back(packed);
	for (uint32_t change : col.changes)
	{
		const int y = static_cast<int>((change >> 16) & 0xFF);
		const int section = y >> 4;
		if (!(col.primary & (1u << section)))
			continue;
		const int index = ((y & 15) << 8) | (static_cast<int>((change >> 24) & 15) << 4) |
		                  static_cast<int>((change >> 28) & 15);
		raw[planeOffset(col.primary, 0, section) + index] = static_cast<int8_t>((change >> 4) & 0xFF);
		int8_t &nib = raw[planeOffset(col.primary, 1, section) + (index >> 1)];
		const uint8_t m = static_cast<uint8_t>(change & 15);
		nib = static_cast<int8_t>((index & 1) ? ((static_cast<uint8_t>(nib) & 0x0F) | (m << 4))
		                                      : ((static_cast<uint8_t>(nib) & 0xF0) | m));
	}
	col.changes.clear();
	deflateColumn(raw, col.z);
}

void Translator340Connection::routeChunk(translator340::ConvertedChunk &chunk)
{
	const int64_t key = columnKey(chunk.x, chunk.z);
	auto it = columns_.find(key);
	if (chunk.groundUp)
	{
		Column &col = columns_[key];
		col.primary = chunk.primary;
		col.changes.clear();
		col.signs.clear();
		deflateColumn(chunk.data, col.z);
		col.live = inWindow(chunk.x, chunk.z, radius_);
		if (!col.live)
			return;
		translator340::Writer pre;
		pre.i32(chunk.x);
		pre.i32(chunk.z);
		pre.u8(1);
		pending_.push_back(decodeNative(0x32, pre.b));
		std::unique_ptr<Packet51MapChunk> map(new Packet51MapChunk());
		map->setDecoded(chunk.x, chunk.z, true, chunk.primary, std::move(chunk.data));
		pending_.push_back(std::move(map));
		return;
	}
	if (it == columns_.end())
		return; // update for a column never received: nothing to merge into
	Column &col = it->second;
	// Merge the updated sections into the stored copy.
	std::vector<int8_t> old;
	if (inflateColumn(col.z, layoutSize(col.primary, true), old))
	{
		const uint16_t merged = static_cast<uint16_t>(col.primary | chunk.primary);
		std::vector<int8_t> raw(layoutSize(merged, true), 0);
		for (int plane = 0; plane < 4; plane++)
			for (int s = 0; s < 16; s++)
			{
				if (!(merged & (1u << s)))
					continue;
				const std::size_t len = plane == 0 ? 4096 : 2048;
				int8_t *dst = &raw[planeOffset(merged, plane, s)];
				if (chunk.primary & (1u << s))
					std::memcpy(dst, &chunk.data[planeOffset(chunk.primary, plane, s)], len);
				else
					std::memcpy(dst, &old[planeOffset(col.primary, plane, s)], len);
			}
		std::memcpy(&raw[layoutSize(merged, false)], &old[layoutSize(col.primary, false)], 256);
		col.primary = merged;
		col.changes.clear();
		deflateColumn(raw, col.z);
	}
	if (!col.live)
		return;
	std::unique_ptr<Packet51MapChunk> map(new Packet51MapChunk());
	map->setDecoded(chunk.x, chunk.z, false, chunk.primary, std::move(chunk.data));
	pending_.push_back(std::move(map));
}

void Translator340Connection::routeNative(translator340::NativePacket &np)
{
	const uint8_t *p = np.payload.data();
	const std::size_t n = np.payload.size();
	if (np.id == 0x01 || np.id == 0x09) // login / respawn: a new world
		columns_.clear();
	if (np.id == 0x35 && n >= 11) // block change
	{
		const int32_t x = readBE32(p), z = readBE32(p + 5);
		const uint8_t y = p[4];
		recordChange(x, y, z, p[9], p[10]);
		auto it = columns_.find(columnKey(x >> 4, z >> 4));
		if (it != columns_.end() && !it->second.live)
			return;
	}
	else if (np.id == 0x34 && n >= 14) // multi block change
	{
		const int32_t cx = readBE32(p), cz = readBE32(p + 4);
		const int count = (p[8] << 8) | p[9];
		for (int i = 0; i < count && 14 + static_cast<std::size_t>(i) * 4 + 4 <= n; i++)
		{
			const uint8_t *r = p + 14 + i * 4;
			const uint16_t coord = static_cast<uint16_t>((r[0] << 8) | r[1]);
			const uint16_t data = static_cast<uint16_t>((r[2] << 8) | r[3]);
			recordChange(cx * 16 + ((coord >> 12) & 15), coord & 0xFF, cz * 16 + ((coord >> 8) & 15),
			             static_cast<uint8_t>(data >> 4), static_cast<uint8_t>(data & 15));
		}
		auto it = columns_.find(columnKey(cx, cz));
		if (it != columns_.end() && !it->second.live)
			return;
	}
	else if (np.id == 0x82 && n >= 10) // sign text: kept with its column
	{
		const int32_t x = readBE32(p), z = readBE32(p + 6);
		const int y = (p[4] << 8) | p[5];
		auto it = columns_.find(columnKey(x >> 4, z >> 4));
		if (it != columns_.end())
		{
			std::vector<std::vector<uint8_t>> &signs = it->second.signs;
			bool replaced = false;
			for (std::vector<uint8_t> &sign : signs)
				if (readBE32(sign.data()) == x && readBE32(sign.data() + 6) == z && ((sign[4] << 8) | sign[5]) == y)
				{
					sign = np.payload;
					replaced = true;
					break;
				}
			if (!replaced && signs.size() < 256)
				signs.push_back(np.payload);
			if (!it->second.live)
				return;
		}
	}
	else if (np.id == 0x32 && n >= 9 && p[8] == 0) // server unload
	{
		auto it = columns_.find(columnKey(readBE32(p), readBE32(p + 4)));
		if (it != columns_.end())
		{
			const bool live = it->second.live;
			columns_.erase(it);
			if (!live)
				return;
		}
	}
	pending_.push_back(decodeNative(np.id, np.payload));
}

void Translator340Connection::updateWindow(bool force)
{
	double px, pz;
	{
		std::lock_guard<std::mutex> guard(session_.lock);
		px = session_.posX;
		pz = session_.posZ;
	}
	const int32_t cx = static_cast<int32_t>(std::floor(px)) >> 4;
	const int32_t cz = static_cast<int32_t>(std::floor(pz)) >> 4;
	if ((++memoryCheck_ & 63) == 0)
	{
		const long freeKb = platformHeapFreeKb();
		if (freeKb >= 0 && freeKb < kShrinkBelowKb && radius_ > kMinRadius)
		{
			radius_--;
			force = true;
			MC_LOG_WARN("net340", "low memory (%ldKB free): chunk window radius -> %d\n", freeKb, radius_);
		}
		else if (freeKb > kGrowAboveKb && radius_ < kMaxRadius)
		{
			radius_++;
			force = true;
		}
	}
	if (!force && cx == centerX_ && cz == centerZ_ && !promotionsPending_)
		return;
	centerX_ = cx;
	centerZ_ = cz;
	int promoted = 0;
	promotionsPending_ = false;
	for (auto &entry : columns_)
	{
		const int32_t x = static_cast<int32_t>(entry.first >> 32);
		const int32_t z = static_cast<int32_t>(static_cast<uint32_t>(entry.first));
		Column &col = entry.second;
		if (col.live && !inWindow(x, z, radius_ + 1))
		{
			translator340::Writer w;
			w.i32(x);
			w.i32(z);
			w.u8(0);
			pending_.push_back(decodeNative(0x32, w.b));
			col.live = false;
		}
		else if (!col.live && inWindow(x, z, radius_))
		{
			if (promoted >= kPromotionsPerRead)
			{
				promotionsPending_ = true;
				continue;
			}
			emitColumn(x, z, col);
			promoted++;
		}
	}
}

void Translator340Connection::emitFrame(std::ostream &os, uint32_t id, const std::vector<uint8_t> &payload)
{
	translator340::Writer body;
	body.varint(id);
	body.raw(payload);
	translator340::Writer frame;
	const int threshold = session_.compressionThreshold.load(std::memory_order_acquire);
	if (threshold >= 0)
	{
		translator340::Writer inner;
		if (body.b.size() >= static_cast<std::size_t>(threshold))
		{
			uLongf bound = compressBound(static_cast<uLong>(body.b.size()));
			std::vector<uint8_t> comp(bound);
			if (compress2(comp.data(), &bound, body.b.data(), static_cast<uLong>(body.b.size()), 6) != Z_OK)
				throw std::runtime_error("1.12.2 compress failed");
			comp.resize(bound);
			inner.varint(static_cast<uint32_t>(body.b.size()));
			inner.raw(comp);
		}
		else
		{
			inner.varint(0);
			inner.raw(body.b);
		}
		frame.varint(static_cast<uint32_t>(inner.b.size()));
		frame.raw(inner.b);
	}
	else
	{
		frame.varint(static_cast<uint32_t>(body.b.size()));
		frame.raw(body.b);
	}
	os.write(reinterpret_cast<const char *>(frame.b.data()), static_cast<std::streamsize>(frame.b.size()));
	if (!os)
		throw std::runtime_error("Failed to write 1.12.2 frame");
}

bool Translator340Connection::flushInjected(std::ostream &os)
{
	bool any = false;
	for (;;)
	{
		std::pair<uint32_t, std::vector<uint8_t>> next;
		{
			std::lock_guard<std::mutex> guard(session_.lock);
			if (session_.injected.empty())
				break;
			next = std::move(session_.injected.front());
			session_.injected.pop_front();
		}
		emitFrame(os, next.first, next.second);
		any = true;
	}
	return any;
}

bool Translator340Connection::writeTranslated(Packet *packet, std::ostream &os)
{
	if (packet == nullptr)
		return false;
	std::ostringstream oss;
	Packet::writePacket(packet, oss);
	const std::string bytes = oss.str();
	if (bytes.empty())
		return false;
	const uint8_t nativeId = static_cast<uint8_t>(bytes[0]);
	const uint8_t *payload = reinterpret_cast<const uint8_t *>(bytes.data()) + 1;
	const std::size_t len = bytes.size() - 1;

	if (session_.stage.load(std::memory_order_acquire) ==
	        static_cast<int>(translator340::Session::Stage::Idle) &&
	    nativeId == 0x02)
	{
		translator340::Cursor c(payload, len);
		std::string username = c.str125(64);
		const std::size_t semi = username.find(';');
		if (semi != std::string::npos)
			username.resize(semi);
		if (username.empty())
			throw std::runtime_error("Bad native handshake packet");
		session_.username = translator340::truncateUtf8(username, 16);
		const std::vector<uint8_t> hs =
		    translator340::encodeHandshake(host_, static_cast<uint16_t>(port_), 2);
		const std::vector<uint8_t> ls = translator340::encodeLoginStart(session_.username);
		os.write(reinterpret_cast<const char *>(hs.data()), static_cast<std::streamsize>(hs.size()));
		os.write(reinterpret_cast<const char *>(ls.data()), static_cast<std::streamsize>(ls.size()));
		if (!os)
			throw std::runtime_error("Failed to write 1.12.2 login");
		session_.stage.store(static_cast<int>(translator340::Session::Stage::Login), std::memory_order_release);
		MC_LOG_INFO("net340", "login start as %s to %s:%d\n", session_.username.c_str(), host_.c_str(), port_);
		return true;
	}

	// Teleport confirms and friends go first: the server ignores movement
	// until the pending teleport is acknowledged.
	bool wrote = flushInjected(os);
	std::vector<std::pair<uint32_t, std::vector<uint8_t>>> out;
	if (translator340::translateServerbound(session_, nativeId, payload, len, out))
	{
		for (const auto &p : out)
			emitFrame(os, p.first, p.second);
		wrote = wrote || !out.empty();
	}
	return wrote;
}

bool Translator340Connection::pollStatus(const std::string &host, int port, std::string &motdOut, int &onlineOut,
                                         int &maxOut, long long &lagMsOut, int &protocolOut)
{
	motdOut.clear();
	onlineOut = maxOut = 0;
	lagMsOut = 0;
	protocolOut = 0;
	std::unique_ptr<JavaNetwork::Socket> socket = JavaNetwork::createSocket();
	if (socket == nullptr || !socket->connect(host, port))
		return false;
	auto readExact = [&](uint8_t *dst, std::size_t n) {
		std::size_t got = 0;
		while (got < n)
		{
			const int just = socket->read(reinterpret_cast<char *>(dst + got), static_cast<int>(n - got));
			if (just <= 0)
				return false;
			got += static_cast<std::size_t>(just);
		}
		return true;
	};
	auto readVarint = [&](uint32_t &v) {
		v = 0;
		for (int i = 0; i < 5; i++)
		{
			uint8_t b = 0;
			if (!readExact(&b, 1))
				return false;
			v |= static_cast<uint32_t>(b & 0x7F) << (7 * i);
			if (!(b & 0x80))
				return true;
		}
		return false;
	};
	const auto start = std::chrono::steady_clock::now();
	std::vector<uint8_t> request = translator340::encodeHandshake(host, static_cast<uint16_t>(port), 1);
	request.push_back(0x01); // status request frame: length 1, id 0
	request.push_back(0x00);
	bool ok = socket->write(reinterpret_cast<const char *>(request.data()), static_cast<int>(request.size()));
	uint32_t total = 0;
	ok = ok && readVarint(total) && total > 0 && total <= 64 * 1024;
	std::vector<uint8_t> frame;
	if (ok)
	{
		frame.resize(total);
		ok = readExact(frame.data(), total);
	}
	if (ok)
	{
		lagMsOut = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start)
		               .count();
		translator340::Cursor c(frame.data(), frame.size());
		ok = c.varint() == 0 && c.ok;
		const std::string json = ok ? c.strMc() : std::string();
		ok = ok && c.ok && translator340::statusToNative(json, motdOut, onlineOut, maxOut, &protocolOut);
	}
	socket->close();
	return ok;
}
