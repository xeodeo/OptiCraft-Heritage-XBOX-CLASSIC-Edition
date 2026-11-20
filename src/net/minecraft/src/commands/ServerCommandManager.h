#pragma once

#include "CommandBase.h"
#include "CommandHandler.h"

// net.minecraft.src.ServerCommandManager (Java 1.5.2, singleplayer/splitscreen)
class ServerCommandManager : public CommandHandler, public IAdminCommand
{
public:
	static ServerCommandManager &getInstance();

	void notifyAdmins(ICommandSender *sender, int flags, const std::string &translationKey, const std::vector<std::string> &args) override;

private:
	ServerCommandManager();
};
