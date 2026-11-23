#pragma once

// ProtocolTranslator340IO -- game-side wiring of the 1.12.2 translator.
//
// Owned by NetworkManager when the connection targets protocol 340:
//   * readOneTranslated() reads 1.12.2 frames (VarInt length, optional zlib
//     block) and returns native 1.2.5 packets built through the regular
//     Packet factory, so handlers, queues and dispatch stay untouched.
//     Chunk columns become Packet51MapChunk objects holding the decoded
//     column directly (no zlib round trip).
//   * writeTranslated() serializes a native packet and emits its 1.12.2
//     equivalent; flushInjected() sends the packets the translator itself
//     needs (teleport confirm, client settings) and is polled by the writer
//     thread, so the reader never writes to the socket.

#include <cstdint>
#include <deque>
#include <map>
#include <vector>
#include <iosfwd>
#include <memory>
#include <string>

#include "ProtocolTranslator340.h"

class Packet;

class Translator340Connection
{
public:
	Translator340Connection(const std::string &host, int port);

	std::unique_ptr<Packet> readOneTranslated(std::istream &is);
	bool writeTranslated(Packet *packet, std::ostream &os);
	bool flushInjected(std::ostream &os);

	// Status ping (server list). protocolOut receives the server's protocol
	// number (Auto mode). Returns false when unreachable or not a 1.7+ server.
	static bool pollStatus(const std::string &host, int port, std::string &motdOut, int &onlineOut,
	                       int &maxOut, long long &lagMsOut, int &protocolOut);

private:
	struct Column
	{
		std::vector<uint8_t> z; // zlib copy of the native layout (with biomes)
		uint16_t primary = 0;
		bool live = false;      // present in the native world
		std::vector<uint32_t> changes; // block changes since the copy was made
		std::vector<std::vector<uint8_t>> signs; // native sign packets of the column
	};

	void emitFrame(std::ostream &os, uint32_t id, const std::vector<uint8_t> &payload);
	void routeNative(translator340::NativePacket &np);
	void routeChunk(translator340::ConvertedChunk &chunk);
	void recordChange(int32_t bx, int32_t by, int32_t bz, uint8_t id, uint8_t meta);
	void emitColumn(int32_t x, int32_t z, Column &col);
	void updateWindow(bool force);
	bool inWindow(int32_t x, int32_t z, int radius) const;

	std::map<int64_t, Column> columns_;
	int32_t centerX_ = 0;
	int32_t centerZ_ = 0;
	bool promotionsPending_ = false;
	int radius_ = 5;
	unsigned memoryCheck_ = 0;

	std::deque<std::unique_ptr<Packet>> pending_;
	std::string host_;
	int port_;
	translator340::Session session_;
	std::vector<uint8_t> frame_;
	std::vector<uint8_t> plain_;
};
