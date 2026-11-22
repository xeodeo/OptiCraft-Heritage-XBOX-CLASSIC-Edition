#pragma once

#include "java/String.h"
#include "java/Type.h"
#include <cstddef>
#include <mutex>
#include <vector>

class NBTTagCompound;

struct ServerActivityEntry
{
	long_t timestamp;
	long_t latency;
	jstring playerCount;
};

// net.minecraft.src.ServerNBTStorage
class ServerNBTStorage
{
public:
	static constexpr std::size_t kMaxActivityEntries = 8;

	ServerNBTStorage(const jstring &name, const jstring &host);

	NBTTagCompound *getCompoundTag() const;
	static ServerNBTStorage *createServerNBTStorage(NBTTagCompound *tag);

	jstring name;
	jstring host;
	jstring playerCount;
	jstring motd;
	long_t lag;
	bool polled;
	std::vector<jstring> playerNames;
	std::vector<ServerActivityEntry> activity;
#ifdef PS2_PLATFORM
	long_t nextPollTime;
	int_t pollRetryCount;
#endif
	mutable std::mutex stateMutex;
};
