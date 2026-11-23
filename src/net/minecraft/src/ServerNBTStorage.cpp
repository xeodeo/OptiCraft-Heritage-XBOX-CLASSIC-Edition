#include "ServerNBTStorage.h"

#include "NBTTagCompound.h"

ServerNBTStorage::ServerNBTStorage(const jstring &serverName, const jstring &serverHost) :
	name(serverName), host(serverHost), playerCount(), motd(), lag(0), version(0), polled(false)
#ifdef PS2_PLATFORM
	, nextPollTime(0), pollRetryCount(0)
#endif
{
}

NBTTagCompound *ServerNBTStorage::getCompoundTag() const
{
	NBTTagCompound *tag = new NBTTagCompound();
	tag->setString("name", name);
	tag->setString("ip", host);
	tag->setInteger("version", version);
	return tag;
}

ServerNBTStorage *ServerNBTStorage::createServerNBTStorage(NBTTagCompound *tag)
{
	if (tag == nullptr)
		return nullptr;
	ServerNBTStorage *server = new ServerNBTStorage(tag->getString("name"), tag->getString("ip"));
	// Entries saved before the version selector were all 1.2.5 servers.
	server->version = tag->hasKey("version") ? tag->getInteger("version") : 29;
	if (server->version != 0 && server->version != 29 && server->version != 340)
		server->version = 0;
	return server;
}
