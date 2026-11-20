#pragma once

#include <string>
#include <vector>
#include "ChunkCoordinates.h"

class EntityPlayer;
class Minecraft;

class ICommandSender
{
public:
	virtual ~ICommandSender() = default;

	virtual std::string getCommandSenderName() const = 0;
	virtual void sendChatToPlayer(const std::string &message) = 0;
	virtual bool canCommandSenderUseCommand(int permissionLevel, const std::string &commandName) const = 0;
	virtual std::string translateString(const std::string &key, const std::vector<std::string> &args = {}) const = 0;
	virtual ChunkCoordinates getPlayerCoordinates() const = 0;
	virtual EntityPlayer *getSenderPlayer() const { return nullptr; }
	virtual Minecraft *getMinecraft() const = 0;
	virtual bool isCommandBlock() const { return false; }
};

class PlayerCommandSender : public ICommandSender
{
public:
	PlayerCommandSender(Minecraft *mc, EntityPlayer *player);

	std::string getCommandSenderName() const override;
	void sendChatToPlayer(const std::string &message) override;
	bool canCommandSenderUseCommand(int permissionLevel, const std::string &commandName) const override;
	std::string translateString(const std::string &key, const std::vector<std::string> &args = {}) const override;
	ChunkCoordinates getPlayerCoordinates() const override;
	EntityPlayer *getSenderPlayer() const override { return player; }
	Minecraft *getMinecraft() const override { return mc; }

private:
	Minecraft *mc;
	EntityPlayer *player;
};
