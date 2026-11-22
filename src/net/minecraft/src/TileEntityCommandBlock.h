#pragma once

#include "TileEntity.h"
#include "commands/ICommandSender.h"
#include <string>
#include <vector>

class NBTTagCompound;
class World;

// net.minecraft.src.TileEntityCommandBlock (Java 1.5.2)
class TileEntityCommandBlock : public TileEntity, public ICommandSender
{
public:
	TileEntityCommandBlock();

	void setCommand(const std::string &cmd);
	const std::string &getCommand() const { return command; }
	int_t executeCommandOnPowered(World *world);

	int_t getSignalStrength() const { return succesCount; }
	void setSignalStrength(int_t strength) { succesCount = strength; }
	void setCommandSenderName(const std::string &name) { commandSenderName = name; }

	// ICommandSender
	std::string getCommandSenderName() const override { return commandSenderName; }
	void sendChatToPlayer(const std::string &message) override;
	bool canCommandSenderUseCommand(int permissionLevel, const std::string &commandName) const override;
	std::string translateString(const std::string &key, const std::vector<std::string> &args = {}) const override;
	ChunkCoordinates getPlayerCoordinates() const override;
	Minecraft *getMinecraft() const override;
	bool isCommandBlock() const override { return true; }

	// TileEntity
	void writeToNBT(NBTTagCompound *nbttagcompound) override;
	void readFromNBT(NBTTagCompound *nbttagcompound) override;

private:
	int_t succesCount;
	std::string command;
	std::string commandSenderName;
};
