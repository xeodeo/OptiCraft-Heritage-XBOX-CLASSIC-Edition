#include "TileEntityCommandBlock.h"

#include "Minecraft.h"
#include "NBTTagCompound.h"
#include "StatCollector.h"
#include "World.h"
#include "commands/ServerCommandManager.h"

TileEntityCommandBlock::TileEntityCommandBlock()
	: succesCount(0)
	, command()
	, commandSenderName("@")
{
}

void TileEntityCommandBlock::setCommand(const std::string &cmd)
{
	command = cmd;
	onInventoryChanged();
}

int_t TileEntityCommandBlock::executeCommandOnPowered(World *world)
{
	if (world == nullptr || world->multiplayerWorld)
		return 0;
	if (command.empty())
		return 0;

	return static_cast<int_t>(ServerCommandManager::getInstance().executeCommand(this, command));
}

void TileEntityCommandBlock::sendChatToPlayer(const std::string &message)
{
	(void)message;
}

bool TileEntityCommandBlock::canCommandSenderUseCommand(int permissionLevel, const std::string &commandName) const
{
	(void)commandName;
	return permissionLevel <= 2;
}

std::string TileEntityCommandBlock::translateString(const std::string &key, const std::vector<std::string> &args) const
{
	std::string result = StatCollector::translateToLocal(key);
	for (const std::string &arg : args)
	{
		const std::size_t pos = result.find("%s");
		if (pos == std::string::npos)
			break;
		result.replace(pos, 2, arg);
	}
	return result;
}

ChunkCoordinates TileEntityCommandBlock::getPlayerCoordinates() const
{
	return ChunkCoordinates(xCoord, yCoord, zCoord);
}

Minecraft *TileEntityCommandBlock::getMinecraft() const
{
	return Minecraft::getMinecraft();
}

void TileEntityCommandBlock::writeToNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::writeToNBT(nbttagcompound);
	nbttagcompound->setString("Command", command);
	nbttagcompound->setInteger("SuccessCount", succesCount);
	nbttagcompound->setString("CustomName", commandSenderName);
}

void TileEntityCommandBlock::readFromNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::readFromNBT(nbttagcompound);
	command = nbttagcompound->getString("Command");
	succesCount = nbttagcompound->getInteger("SuccessCount");
	if (nbttagcompound->hasKey("CustomName"))
		commandSenderName = nbttagcompound->getString("CustomName");
	else
		commandSenderName = "@";
}
