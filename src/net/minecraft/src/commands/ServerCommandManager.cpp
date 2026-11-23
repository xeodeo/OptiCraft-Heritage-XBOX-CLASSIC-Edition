#include "ServerCommandManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "ChunkCoordinates.h"
#include "CommandBase.h"
#include "CommandExceptions.h"
#include "Container.h"
#include "DamageSource.h"
#include "Enchantment.h"
#include "EntityItem.h"
#include "EntityPlayer.h"
#include "EntityPlayerSP.h"
#include "EnumGameType.h"
#include "GameRules.h"
#include "GameSettings.h"
#include "GuiIngame.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "MathHelper.h"
#include "Minecraft.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "PlayerSelector.h"
#include "StatCollector.h"
#include "World.h"
#include "WorldInfo.h"
#include "scoreboard/Score.h"
#include "scoreboard/ScoreObjective.h"
#include "scoreboard/ScoreObjectiveCriteria.h"
#include "scoreboard/ScorePlayerTeam.h"
#include "scoreboard/Scoreboard.h"
#include "java/Random.h"
#include "java/String.h"
#include "platform/Log.h"

namespace
{
const std::string kChatDarkGreen = "\xc2\xa7" "2";
const std::string kChatGreen = "\xc2\xa7" "a";
const std::string kChatGrayItalic = "\xc2\xa7" "7" "\xc2\xa7" "o";

// ─── /time ───────────────────────────────────────────────────────────────────
class CommandTime : public CommandBase
{
public:
	std::string getCommandName() const override { return "time"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.time.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		Minecraft *mc = sender->getMinecraft();
		if (args.size() > 1 && mc != nullptr && mc->theWorld != nullptr)
		{
			if (args[0] == "set")
			{
				int val = 0;
				if (args[1] == "day")
					val = 0;
				else if (args[1] == "night")
					val = 12500;
				else
					val = parseIntWithMin(sender, args[1], 0);
				mc->theWorld->setWorldTime(static_cast<long_t>(val));
				notifyAdmins(sender, "commands.time.set", {std::to_string(val)});
				return;
			}
			if (args[0] == "add")
			{
				const int val = parseIntWithMin(sender, args[1], 0);
				mc->theWorld->setWorldTime(mc->theWorld->getWorldTime() + static_cast<long_t>(val));
				notifyAdmins(sender, "commands.time.added", {std::to_string(val)});
				return;
			}
		}
		throw WrongUsageException("commands.time.usage");
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		(void)sender;
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, {"set", "add"});
		if (args.size() == 2 && args[0] == "set")
			return getListOfStringsMatchingLastWord(args, {"day", "night"});
		return {};
	}
};

// ─── /gamemode ───────────────────────────────────────────────────────────────
class CommandGameMode : public CommandBase
{
public:
	std::string getCommandName() const override { return "gamemode"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.gamemode.usage"); }

	static int_t getGameModeFromCommand(ICommandSender *sender, const std::string &token)
	{
		const std::string lower = String::toLowerCaseJava(token);
		if (lower == "survival" || lower == "s")
			return EnumGameType::SURVIVAL;
		if (lower == "creative" || lower == "c")
			return EnumGameType::CREATIVE;
		if (lower == "adventure" || lower == "a")
			return EnumGameType::ADVENTURE;
		return EnumGameType::getByID(parseIntBounded(sender, token, 0, 2));
	}

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.gamemode.usage");

		const int_t mode = getGameModeFromCommand(sender, args[0]);
		EntityPlayer *target = (args.size() >= 2) ? getPlayer(sender, args[1]) : getCommandSenderAsPlayer(sender);
		Minecraft *mc = sender->getMinecraft();
		if (mc != nullptr)
			mc->setPlayerGameType(target, mode);
		target->fallDistance = 0.0f;

		const std::string localizedMode = StatCollector::translateToLocal("gameMode." + EnumGameType::getName(mode));
		if (target != sender->getSenderPlayer())
			notifyAdmins(sender, 1, "commands.gamemode.success.other", {target->username, localizedMode});
		else
			notifyAdmins(sender, 1, "commands.gamemode.success.self", {localizedMode});
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, {"survival", "creative", "adventure"});
		if (args.size() == 2)
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 1;
	}
};

// ─── /defaultgamemode ────────────────────────────────────────────────────────
class CommandDefaultGameMode : public CommandGameMode
{
public:
	std::string getCommandName() const override { return "defaultgamemode"; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.defaultgamemode.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.defaultgamemode.usage");

		const int_t mode = getGameModeFromCommand(sender, args[0]);
		Minecraft *mc = sender->getMinecraft();
		if (mc != nullptr && mc->theWorld != nullptr && mc->theWorld->getWorldInfo() != nullptr)
			mc->theWorld->getWorldInfo()->setGameType(mode);

		const std::string localizedMode = StatCollector::translateToLocal("gameMode." + EnumGameType::getName(mode));
		notifyAdmins(sender, "commands.defaultgamemode.success", {localizedMode});
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		(void)sender;
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, {"survival", "creative", "adventure"});
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		(void)index;
		return false;
	}
};

// ─── /difficulty ─────────────────────────────────────────────────────────────
class CommandDifficulty : public CommandBase
{
public:
	std::string getCommandName() const override { return "difficulty"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.difficulty.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.difficulty.usage");

		static const char *kDifficulties[4] = {
			"options.difficulty.peaceful",
			"options.difficulty.easy",
			"options.difficulty.normal",
			"options.difficulty.hard"
		};

		const std::string lower = String::toLowerCaseJava(args[0]);
		int diff = 0;
		if (lower == "peaceful" || lower == "p")
			diff = 0;
		else if (lower == "easy" || lower == "e")
			diff = 1;
		else if (lower == "normal" || lower == "n")
			diff = 2;
		else if (lower == "hard" || lower == "h")
			diff = 3;
		else
			diff = parseIntBounded(sender, args[0], 0, 3);

		Minecraft *mc = sender->getMinecraft();
		if (mc != nullptr)
		{
			if (mc->gameSettings != nullptr)
				mc->gameSettings->difficulty = diff;
			if (mc->theWorld != nullptr)
				mc->theWorld->difficultySetting = diff;
		}
		const std::string localizedDiff = StatCollector::translateToLocal(kDifficulties[diff]);
		notifyAdmins(sender, "commands.difficulty.success", {localizedDiff});
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		(void)sender;
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, {"peaceful", "easy", "normal", "hard"});
		return {};
	}
};

// ─── /kill ───────────────────────────────────────────────────────────────────
class CommandKill : public CommandBase
{
public:
	std::string getCommandName() const override { return "kill"; }
	int getRequiredPermissionLevel() const override { return 0; }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		(void)args;
		EntityPlayer *player = getCommandSenderAsPlayer(sender);
		player->attackEntityFrom(DamageSource::outOfWorld, 1000);
		sender->sendChatToPlayer("Ouch. That looks like it hurt.");
	}
};

// ─── /toggledownfall ─────────────────────────────────────────────────────────
class CommandToggleDownfall : public CommandBase
{
public:
	std::string getCommandName() const override { return "toggledownfall"; }
	int getRequiredPermissionLevel() const override { return 2; }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		(void)args;
		Minecraft *mc = sender->getMinecraft();
		if (mc != nullptr && mc->theWorld != nullptr)
		{
			mc->theWorld->toggleRain();
			if (mc->theWorld->getWorldInfo() != nullptr)
				mc->theWorld->getWorldInfo()->setThundering(true);
		}
		notifyAdmins(sender, "commands.downfall.success");
	}
};

// ─── /weather ────────────────────────────────────────────────────────────────
class CommandWeather : public CommandBase
{
public:
	std::string getCommandName() const override { return "weather"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.weather.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.weather.usage");

		static Random s_rand;
		int durationTicks = (300 + s_rand.nextInt(600)) * 20;
		if (args.size() >= 2)
			durationTicks = parseIntBounded(sender, args[1], 1, 1000000) * 20;

		Minecraft *mc = sender->getMinecraft();
		WorldInfo *info = (mc != nullptr && mc->theWorld != nullptr) ? mc->theWorld->getWorldInfo() : nullptr;
		const std::string mode = String::toLowerCaseJava(args[0]);
		if (mode == "clear")
		{
			if (info != nullptr)
			{
				info->setRainTime(durationTicks);
				info->setThunderTime(durationTicks);
				info->setRaining(false);
				info->setThundering(false);
			}
			notifyAdmins(sender, "commands.weather.clear");
		}
		else if (mode == "rain")
		{
			if (info != nullptr)
			{
				info->setRainTime(durationTicks);
				info->setThunderTime(durationTicks);
				info->setRaining(true);
				info->setThundering(false);
			}
			notifyAdmins(sender, "commands.weather.rain");
		}
		else if (mode == "thunder")
		{
			if (info != nullptr)
			{
				info->setRainTime(durationTicks);
				info->setThunderTime(durationTicks);
				info->setRaining(true);
				info->setThundering(true);
			}
			notifyAdmins(sender, "commands.weather.thunder");
		}
		else
		{
			throw WrongUsageException("commands.weather.usage");
		}
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		(void)sender;
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, {"clear", "rain", "thunder"});
		return {};
	}
};

// ─── /xp ─────────────────────────────────────────────────────────────────────
class CommandXP : public CommandBase
{
public:
	std::string getCommandName() const override { return "xp"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.xp.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.xp.usage");

		std::string amtStr = args[0];
		const bool isLevels = !amtStr.empty() && (amtStr.back() == 'l' || amtStr.back() == 'L');
		if (isLevels && amtStr.size() > 1)
			amtStr.pop_back();

		int amount = parseInt(sender, amtStr);
		const bool isNegative = amount < 0;
		if (isNegative)
			amount = -amount;

		EntityPlayer *target = (args.size() > 1) ? getPlayer(sender, args[1]) : getCommandSenderAsPlayer(sender);
		if (isLevels)
		{
			if (isNegative)
			{
				target->addExperienceLevel(-amount);
				notifyAdmins(sender, "commands.xp.success.negative.levels", {std::to_string(amount), target->username});
			}
			else
			{
				target->addExperienceLevel(amount);
				notifyAdmins(sender, "commands.xp.success.levels", {std::to_string(amount), target->username});
			}
		}
		else
		{
			if (isNegative)
				throw WrongUsageException("commands.xp.failure.widthdrawXp");
			target->addExperience(amount);
			notifyAdmins(sender, "commands.xp.success", {std::to_string(amount), target->username});
		}
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 2)
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 1;
	}
};

// ─── /tp ─────────────────────────────────────────────────────────────────────
class CommandServerTp : public CommandBase
{
public:
	std::string getCommandName() const override { return "tp"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.tp.usage"); }

	static double parseCoordinate(ICommandSender *sender, double baseVal, const std::string &token, int minBound, int maxBound)
	{
		const bool relative = !token.empty() && token[0] == '~';
		double result = relative ? baseVal : 0.0;
		if (!relative || token.size() > 1)
		{
			const bool hasDecimal = (token.find('.') != std::string::npos);
			const std::string numPart = relative ? token.substr(1) : token;
			result += parseDouble(sender, numPart);
			if (!hasDecimal && !relative)
				result += 0.5;
		}
		if (minBound != 0 || maxBound != 0)
		{
			if (result < static_cast<double>(minBound))
				throw NumberInvalidException("commands.generic.double.tooSmall", {formatDouble2(result), formatDouble2(static_cast<double>(minBound))});
			if (result > static_cast<double>(maxBound))
				throw NumberInvalidException("commands.generic.double.tooBig", {formatDouble2(result), formatDouble2(static_cast<double>(maxBound))});
		}
		return result;
	}

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.tp.usage");

		EntityPlayer *target = nullptr;
		if (args.size() != 2 && args.size() != 4)
			target = getCommandSenderAsPlayer(sender);
		else
			target = getPlayer(sender, args[0]);

		if (args.size() == 3 || args.size() == 4)
		{
			if (target->worldObj != nullptr)
			{
				const std::size_t base = args.size() - 3;
				const double curFeetY = target->boundingBox != nullptr ? target->boundingBox->minY : (target->posY - target->yOffset);
				const double nx = parseCoordinate(sender, target->posX, args[base], -30000000, 30000000);
				const double ny = parseCoordinate(sender, curFeetY, args[base + 1], 0, 0);
				const double nz = parseCoordinate(sender, target->posZ, args[base + 2], -30000000, 30000000);
				target->mountEntity(nullptr);
				target->setLocationAndAngles(nx, ny, nz, target->rotationYaw, target->rotationPitch);
				target->motionX = target->motionY = target->motionZ = 0.0;
				target->fallDistance = 0.0f;
				notifyAdmins(sender, "commands.tp.success.coordinates",
					{target->username, formatDouble2(nx), formatDouble2(ny), formatDouble2(nz)});
			}
		}
		else if (args.size() == 1 || args.size() == 2)
		{
			EntityPlayer *dest = getPlayer(sender, args.back());
			if (dest->worldObj != target->worldObj)
			{
				notifyAdmins(sender, "commands.tp.notSameDimension");
				return;
			}
			const double destFeetY = dest->boundingBox != nullptr ? dest->boundingBox->minY : (dest->posY - dest->yOffset);
			target->mountEntity(nullptr);
			target->setLocationAndAngles(dest->posX, destFeetY, dest->posZ, dest->rotationYaw, dest->rotationPitch);
			target->motionX = target->motionY = target->motionZ = 0.0;
			target->fallDistance = 0.0f;
			notifyAdmins(sender, "commands.tp.success", {target->username, dest->username});
		}
		else
		{
			throw WrongUsageException("commands.tp.usage");
		}
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 1 || args.size() == 2)
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 0;
	}
};

// ─── /give ───────────────────────────────────────────────────────────────────
class CommandGive : public CommandBase
{
public:
	std::string getCommandName() const override { return "give"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.give.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.size() < 2)
			throw WrongUsageException("commands.give.usage");

		EntityPlayer *target = getPlayer(sender, args[0]);
		const int itemId = parseIntWithMin(sender, args[1], 1);
		int count = 1;
		int damage = 0;

		if (!ItemStack::isValidItemID(itemId))
			throw NumberInvalidException("commands.give.notFound", {std::to_string(itemId)});

		if (args.size() >= 3)
			count = parseIntBounded(sender, args[2], 1, 64);
		if (args.size() >= 4)
			damage = parseInt(sender, args[3]);

		ItemStack *stack = new ItemStack(itemId, count, damage);
		const std::string itemDisplay = stack->getDisplayName();
		EntityItem *dropped = target->dropPlayerItem(stack);
		if (dropped != nullptr)
		{
			const double feetY = target->boundingBox != nullptr ? target->boundingBox->minY : (target->posY - target->yOffset);
			dropped->setPosition(target->posX, feetY + 0.5, target->posZ);
			dropped->motionX = 0.0;
			dropped->motionY = 0.0;
			dropped->motionZ = 0.0;
			dropped->delayBeforeCanPickup = 0;
			dropped->onCollideWithPlayer(target);
		}
		notifyAdmins(sender, "commands.give.success",
			{itemDisplay, std::to_string(itemId), std::to_string(count), target->username});
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 0;
	}
};

// ─── /enchant ────────────────────────────────────────────────────────────────
class CommandEnchant : public CommandBase
{
public:
	std::string getCommandName() const override { return "enchant"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.enchant.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.size() < 2)
			throw WrongUsageException("commands.enchant.usage");

		Enchantment::initEnchantments();
		EntityPlayer *target = getPlayer(sender, args[0]);
		const int enchId = parseIntBounded(sender, args[1], 0, 255);
		int level = 1;

		ItemStack *held = target->getCurrentEquippedItem();
		if (held == nullptr)
		{
			notifyAdmins(sender, "commands.enchant.noItem");
			return;
		}

		Enchantment *ench = Enchantment::enchantmentsList[enchId];
		if (ench == nullptr)
			throw NumberInvalidException("commands.enchant.notFound", {std::to_string(enchId)});

		if (!ench->canApply(held))
		{
			notifyAdmins(sender, "commands.enchant.cantEnchant");
			return;
		}

		if (args.size() >= 3)
			level = parseIntBounded(sender, args[2], ench->getMinLevel(), ench->getMaxLevel());

		if (held->hasTagCompound())
		{
			NBTTagList *list = held->getEnchantmentTagList();
			if (list != nullptr)
			{
				for (int i = 0; i < list->tagCount(); ++i)
				{
					NBTTagCompound *entry = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
					if (entry == nullptr)
						continue;
					const short existingId = entry->getShort("id");
					if (existingId >= 0 && existingId < 256 && Enchantment::enchantmentsList[existingId] != nullptr)
					{
						Enchantment *existing = Enchantment::enchantmentsList[existingId];
						if (!existing->canApplyTogether(ench))
						{
							notifyAdmins(sender, "commands.enchant.cantCombine",
								{ench->getTranslatedName(level), existing->getTranslatedName(entry->getShort("lvl"))});
							return;
						}
					}
				}
			}
		}

		held->addEnchantment(ench, level);
		notifyAdmins(sender, "commands.enchant.success");
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 0;
	}
};

// ─── /me ─────────────────────────────────────────────────────────────────────
class CommandServerEmote : public CommandBase
{
public:
	std::string getCommandName() const override { return "me"; }
	int getRequiredPermissionLevel() const override { return 0; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.me.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.me.usage");
		const std::string text = joinArgs(sender, args, 0, sender->canCommandSenderUseCommand(1, "me"));
		sender->sendChatToPlayer("* " + sender->getCommandSenderName() + " " + text);
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
	}
};

// ─── /say ────────────────────────────────────────────────────────────────────
class CommandServerSay : public CommandBase
{
public:
	std::string getCommandName() const override { return "say"; }
	int getRequiredPermissionLevel() const override { return 1; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.say.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty() || args[0].empty())
			throw WrongUsageException("commands.say.usage");
		const std::string text = joinArgs(sender, args, 0, true);
		sender->sendChatToPlayer("[" + sender->getCommandSenderName() + "] " + text);
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (!args.empty())
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}
};

// ─── /tell (aliases: w, msg) ─────────────────────────────────────────────────
class CommandServerMessage : public CommandBase
{
public:
	std::string getCommandName() const override { return "tell"; }
	std::vector<std::string> getCommandAliases() const override { return {"w", "msg"}; }
	int getRequiredPermissionLevel() const override { return 0; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.message.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.size() < 2)
			throw WrongUsageException("commands.message.usage");
		EntityPlayer *target = getPlayer(sender, args[0]);
		if (target == sender->getSenderPlayer())
			throw PlayerNotFoundException("commands.message.sameTarget");

		const std::string text = joinArgs(sender, args, 1, sender->getSenderPlayer() == nullptr);
		// In local splitscreen both players share the HUD chat log; show incoming/outgoing cleanly.
		sender->sendChatToPlayer(kChatGrayItalic + sender->translateString("commands.message.display.outgoing", {target->username, text}));
		if (target != sender->getSenderPlayer())
			sender->sendChatToPlayer(kChatGrayItalic + sender->translateString("commands.message.display.incoming", {sender->getCommandSenderName(), text}));
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 0;
	}
};

// ─── /seed ───────────────────────────────────────────────────────────────────
class CommandShowSeed : public CommandBase
{
public:
	std::string getCommandName() const override { return "seed"; }
	int getRequiredPermissionLevel() const override { return 2; }

	bool canCommandSenderUseCommand(ICommandSender *sender) const override
	{
		// Java 1.5.2: singleplayer always allows /seed even when cheats are off.
		Minecraft *mc = sender != nullptr ? sender->getMinecraft() : nullptr;
		if (mc != nullptr && !mc->isMultiplayerWorld())
			return true;
		return CommandBase::canCommandSenderUseCommand(sender);
	}

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		(void)args;
		Minecraft *mc = sender->getMinecraft();
		const long_t seed = (mc != nullptr && mc->theWorld != nullptr) ? mc->theWorld->getRandomSeed() : 0LL;
		sender->sendChatToPlayer("Seed: " + std::to_string(seed));
	}
};

// ─── /spawnpoint ─────────────────────────────────────────────────────────────
class CommandSetSpawnpoint : public CommandBase
{
public:
	std::string getCommandName() const override { return "spawnpoint"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.spawnpoint.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		EntityPlayer *target = args.empty() ? getCommandSenderAsPlayer(sender) : getPlayer(sender, args[0]);
		if (args.size() == 4)
		{
			if (target->worldObj != nullptr)
			{
				const int x = parseIntBounded(sender, args[1], -30000000, 30000000);
				const int y = parseIntBounded(sender, args[2], 0, 256);
				const int z = parseIntBounded(sender, args[3], -30000000, 30000000);
				ChunkCoordinates cc(x, y, z);
				target->setSpawnChunk(&cc, true);
				notifyAdmins(sender, "commands.spawnpoint.success",
					{target->username, std::to_string(x), std::to_string(y), std::to_string(z)});
			}
		}
		else if (args.size() <= 1)
		{
			ChunkCoordinates cc(
				MathHelper::floor_double(target->posX),
				MathHelper::floor_double(target->boundingBox != nullptr ? target->boundingBox->minY : target->posY),
				MathHelper::floor_double(target->posZ));
			target->setSpawnChunk(&cc, true);
			notifyAdmins(sender, "commands.spawnpoint.success",
				{target->username, std::to_string(cc.x), std::to_string(cc.y), std::to_string(cc.z)});
		}
		else
		{
			throw WrongUsageException("commands.spawnpoint.usage");
		}
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 1 || args.size() == 2)
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 0;
	}
};

// ─── /gamerule ───────────────────────────────────────────────────────────────
class CommandGameRule : public CommandBase
{
public:
	std::string getCommandName() const override { return "gamerule"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.gamerule.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		Minecraft *mc = sender->getMinecraft();
		if (mc == nullptr || mc->theWorld == nullptr)
			return;
		GameRules &rules = mc->theWorld->getGameRules();

		if (args.size() == 2)
		{
			const std::string &name = args[0];
			const std::string &val = args[1];
			if (rules.hasRule(name))
			{
				rules.setOrCreateGameRule(name, val);
				notifyAdmins(sender, "commands.gamerule.success");
			}
			else
			{
				notifyAdmins(sender, "commands.gamerule.norule", {name});
			}
		}
		else if (args.size() == 1)
		{
			const std::string &name = args[0];
			if (rules.hasRule(name))
				sender->sendChatToPlayer(name + " = " + rules.getGameRuleStringValue(name));
			else
				notifyAdmins(sender, "commands.gamerule.norule", {name});
		}
		else if (args.empty())
		{
			sender->sendChatToPlayer(joinNiceString(rules.getRules()));
		}
		else
		{
			throw WrongUsageException("commands.gamerule.usage");
		}
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		Minecraft *mc = sender->getMinecraft();
		if (args.size() == 1 && mc != nullptr && mc->theWorld != nullptr)
			return getListOfStringsMatchingLastWord(args, mc->theWorld->getGameRules().getRules());
		if (args.size() == 2)
			return getListOfStringsMatchingLastWord(args, {"true", "false"});
		return {};
	}
};

// ─── /clear ──────────────────────────────────────────────────────────────────
class CommandClearInventory : public CommandBase
{
public:
	std::string getCommandName() const override { return "clear"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.clear.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		EntityPlayer *target = args.empty() ? getCommandSenderAsPlayer(sender) : getPlayer(sender, args[0]);
		const int itemId = (args.size() >= 2) ? parseIntWithMin(sender, args[1], 1) : -1;
		const int metadata = (args.size() >= 3) ? parseIntWithMin(sender, args[2], 0) : -1;
		const int removed = (target->inventory != nullptr) ? target->inventory->clearInventory(itemId, metadata) : 0;
		if (target->inventorySlots != nullptr)
			target->inventorySlots->detectAndSendChanges();
		if (removed == 0)
			throw CommandException("commands.clear.failure", {target->username});
		notifyAdmins(sender, "commands.clear.success", {target->username, std::to_string(removed)});
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		return {};
	}

	bool isUsernameIndex(const std::vector<std::string> &args, int index) const override
	{
		(void)args;
		return index == 0;
	}
};

// ─── /scoreboard ─────────────────────────────────────────────────────────────
class CommandScoreboard : public CommandBase
{
public:
	std::string getCommandName() const override { return "scoreboard"; }
	int getRequiredPermissionLevel() const override { return 2; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.scoreboard.usage"); }

	static std::string colorCodeByName(const std::string &name)
	{
		const std::string lower = String::toLowerCaseJava(name);
		if (lower == "black")        return "\xc2\xa7" "0";
		if (lower == "dark_blue")    return "\xc2\xa7" "1";
		if (lower == "dark_green")   return "\xc2\xa7" "2";
		if (lower == "dark_aqua")    return "\xc2\xa7" "3";
		if (lower == "dark_red")     return "\xc2\xa7" "4";
		if (lower == "dark_purple")  return "\xc2\xa7" "5";
		if (lower == "gold")         return "\xc2\xa7" "6";
		if (lower == "gray")         return "\xc2\xa7" "7";
		if (lower == "dark_gray")    return "\xc2\xa7" "8";
		if (lower == "blue")         return "\xc2\xa7" "9";
		if (lower == "green")        return "\xc2\xa7" "a";
		if (lower == "aqua")         return "\xc2\xa7" "b";
		if (lower == "red")          return "\xc2\xa7" "c";
		if (lower == "light_purple") return "\xc2\xa7" "d";
		if (lower == "yellow")       return "\xc2\xa7" "e";
		if (lower == "white")        return "\xc2\xa7" "f";
		if (lower == "reset")        return "\xc2\xa7" "r";
		return "";
	}

	static std::vector<std::string> validColorNames()
	{
		return {
			"black", "dark_blue", "dark_green", "dark_aqua", "dark_red", "dark_purple",
			"gold", "gray", "dark_gray", "blue", "green", "aqua",
			"red", "light_purple", "yellow", "white", "reset"
		};
	}

	static Scoreboard *getBoard(ICommandSender *sender)
	{
		Minecraft *mc = sender != nullptr ? sender->getMinecraft() : nullptr;
		if (mc == nullptr || mc->theWorld == nullptr)
			throw CommandException("commands.scoreboard.usage");
		return mc->theWorld->getScoreboard();
	}

	static ScoreObjective *requireObjective(ICommandSender *sender, const std::string &name, bool requireWritable)
	{
		Scoreboard *board = getBoard(sender);
		ScoreObjective *obj = board->getObjective(name);
		if (obj == nullptr)
			throw CommandException("commands.scoreboard.objectiveNotFound", {name});
		if (requireWritable && obj->getCriteria() != nullptr && obj->getCriteria()->isReadOnly())
			throw CommandException("commands.scoreboard.objectiveReadOnly", {name});
		return obj;
	}

	static ScorePlayerTeam *requireTeam(ICommandSender *sender, const std::string &name)
	{
		Scoreboard *board = getBoard(sender);
		ScorePlayerTeam *team = board->getTeam(name);
		if (team == nullptr)
			throw CommandException("commands.scoreboard.teamNotFound", {name});
		return team;
	}

	static std::string resolveTargetName(ICommandSender *sender, const std::string &token)
	{
		if (PlayerSelector::hasArguments(token))
		{
			EntityPlayer *matched = PlayerSelector::matchOnePlayer(sender, token);
			if (matched == nullptr)
				throw PlayerNotFoundException();
			return matched->username;
		}
		return token;
	}

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		if (args.empty())
			throw WrongUsageException("commands.scoreboard.usage");

		const std::string sub = String::toLowerCaseJava(args[0]);
		if (sub == "objectives")
		{
			if (args.size() == 1)
				throw WrongUsageException("commands.scoreboard.objectives.usage");
			const std::string act = String::toLowerCaseJava(args[1]);
			if (act == "list")
			{
				listObjectives(sender);
			}
			else if (act == "add")
			{
				if (args.size() < 4)
					throw WrongUsageException("commands.scoreboard.objectives.add.usage");
				addObjective(sender, args);
			}
			else if (act == "remove")
			{
				if (args.size() != 3)
					throw WrongUsageException("commands.scoreboard.objectives.remove.usage");
				removeObjective(sender, args[2]);
			}
			else if (act == "setdisplay")
			{
				if (args.size() != 3 && args.size() != 4)
					throw WrongUsageException("commands.scoreboard.objectives.setdisplay.usage");
				setDisplay(sender, args);
			}
			else
			{
				throw WrongUsageException("commands.scoreboard.objectives.usage");
			}
			return;
		}

		if (sub == "players")
		{
			if (args.size() == 1)
				throw WrongUsageException("commands.scoreboard.players.usage");
			const std::string act = String::toLowerCaseJava(args[1]);
			if (act == "list")
			{
				if (args.size() > 3)
					throw WrongUsageException("commands.scoreboard.players.list.usage");
				listPlayers(sender, args);
			}
			else if (act == "add" || act == "remove" || act == "set")
			{
				if (args.size() != 5)
					throw WrongUsageException("commands.scoreboard.players." + act + ".usage");
				modifyPlayerScore(sender, act, args);
			}
			else if (act == "reset")
			{
				if (args.size() != 3)
					throw WrongUsageException("commands.scoreboard.players.reset.usage");
				const std::string target = resolveTargetName(sender, args[2]);
				getBoard(sender)->resetPlayerScores(target);
				notifyAdmins(sender, "commands.scoreboard.players.reset.success", {target});
			}
			else
			{
				throw WrongUsageException("commands.scoreboard.players.usage");
			}
			return;
		}

		if (sub == "teams")
		{
			if (args.size() == 1)
				throw WrongUsageException("commands.scoreboard.teams.usage");
			const std::string act = String::toLowerCaseJava(args[1]);
			if (act == "list")
			{
				if (args.size() > 3)
					throw WrongUsageException("commands.scoreboard.teams.list.usage");
				listTeams(sender, args);
			}
			else if (act == "add")
			{
				if (args.size() < 3)
					throw WrongUsageException("commands.scoreboard.teams.add.usage");
				addTeam(sender, args);
			}
			else if (act == "remove")
			{
				if (args.size() != 3)
					throw WrongUsageException("commands.scoreboard.teams.remove.usage");
				ScorePlayerTeam *team = requireTeam(sender, args[2]);
				const std::string name = team->getRegisteredName();
				getBoard(sender)->removeTeam(team);
				notifyAdmins(sender, "commands.scoreboard.teams.remove.success", {name});
			}
			else if (act == "empty")
			{
				if (args.size() != 3)
					throw WrongUsageException("commands.scoreboard.teams.empty.usage");
				Scoreboard *board = getBoard(sender);
				ScorePlayerTeam *team = requireTeam(sender, args[2]);
				std::vector<std::string> members(team->getMembershipCollection().begin(),
				                                 team->getMembershipCollection().end());
				if (members.empty())
					throw CommandException("commands.scoreboard.teams.empty.alreadyEmpty", {team->getRegisteredName()});
				for (const std::string &m : members)
					board->removePlayerFromTeam(m, team);
				notifyAdmins(sender, "commands.scoreboard.teams.empty.success",
					{std::to_string(members.size()), team->getRegisteredName()});
			}
			else if (act == "join")
			{
				if (args.size() < 4 && (args.size() != 3 || sender->getSenderPlayer() == nullptr))
					throw WrongUsageException("commands.scoreboard.teams.join.usage");
				joinTeam(sender, args);
			}
			else if (act == "leave")
			{
				if (args.size() < 3 && sender->getSenderPlayer() == nullptr)
					throw WrongUsageException("commands.scoreboard.teams.leave.usage");
				leaveTeam(sender, args);
			}
			else if (act == "option")
			{
				if (args.size() != 4 && args.size() != 5)
					throw WrongUsageException("commands.scoreboard.teams.option.usage");
				setTeamOption(sender, args);
			}
			else
			{
				throw WrongUsageException("commands.scoreboard.teams.usage");
			}
			return;
		}

		throw WrongUsageException("commands.scoreboard.usage");
	}

	std::vector<std::string> addTabCompletionOptions(ICommandSender *sender, const std::vector<std::string> &args) const override
	{
		if (args.size() == 1)
			return getListOfStringsMatchingLastWord(args, {"objectives", "players", "teams"});
		const std::string sub = String::toLowerCaseJava(args[0]);
		if (sub == "objectives")
		{
			if (args.size() == 2)
				return getListOfStringsMatchingLastWord(args, {"list", "add", "remove", "setdisplay"});
			const std::string act = String::toLowerCaseJava(args[1]);
			if (act == "add" && args.size() == 4)
			{
				std::vector<std::string> crits;
				for (const auto &e : ScoreObjectiveCriteria::getCriteriaMap())
					crits.push_back(e.first);
				return getListOfStringsMatchingLastWord(args, crits);
			}
			if (act == "setdisplay" && args.size() == 3)
				return getListOfStringsMatchingLastWord(args, {"list", "sidebar", "belowName"});
		}
		else if (sub == "players")
		{
			if (args.size() == 2)
				return getListOfStringsMatchingLastWord(args, {"set", "add", "remove", "reset", "list"});
			if (args.size() == 3)
				return getListOfStringsMatchingLastWord(args, getAllUsernames(sender->getMinecraft()));
		}
		else if (sub == "teams")
		{
			if (args.size() == 2)
				return getListOfStringsMatchingLastWord(args, {"add", "remove", "join", "leave", "empty", "list", "option"});
			if (String::equalsIgnoreCaseJava(args[1], "option"))
			{
				if (args.size() == 4)
					return getListOfStringsMatchingLastWord(args, {"color", "friendlyfire", "seeFriendlyInvisibles"});
				if (args.size() == 5)
				{
					if (String::equalsIgnoreCaseJava(args[3], "color"))
						return getListOfStringsMatchingLastWord(args, validColorNames());
					return getListOfStringsMatchingLastWord(args, {"true", "false"});
				}
			}
		}
		return {};
	}

private:
	static void listObjectives(ICommandSender *sender)
	{
		Scoreboard *board = getBoard(sender);
		const std::vector<ScoreObjective *> objs = board->getScoreObjectives();
		if (objs.empty())
			throw CommandException("commands.scoreboard.objectives.list.empty");
		sender->sendChatToPlayer(kChatDarkGreen + sender->translateString("commands.scoreboard.objectives.list.count", {std::to_string(objs.size())}));
		for (ScoreObjective *obj : objs)
		{
			sender->sendChatToPlayer(sender->translateString("commands.scoreboard.objectives.list.entry",
				{obj->getName(), obj->getDisplayName(), obj->getCriteria()->getName()}));
		}
	}

	static void addObjective(ICommandSender *sender, const std::vector<std::string> &args)
	{
		const std::string &name = args[2];
		const std::string &critName = args[3];
		Scoreboard *board = getBoard(sender);
		ScoreObjectiveCriteria *criteria = ScoreObjectiveCriteria::getByName(critName);
		if (criteria == nullptr)
		{
			std::vector<std::string> valid;
			for (const auto &e : ScoreObjectiveCriteria::getCriteriaMap())
				valid.push_back(e.first);
			throw WrongUsageException("commands.scoreboard.objectives.add.wrongType", {joinNiceString(valid)});
		}
		if (board->getObjective(name) != nullptr)
			throw CommandException("commands.scoreboard.objectives.add.alreadyExists", {name});
		if (name.size() > 16)
			throw CommandException("commands.scoreboard.objectives.add.tooLong", {name, "16"});

		ScoreObjective *obj = board->addScoreObjective(name, criteria);
		if (args.size() > 4)
		{
			const std::string display = joinArgs(sender, args, 4, false);
			if (display.size() > 32)
			{
				board->removeObjective(obj);
				throw CommandException("commands.scoreboard.objectives.add.displayTooLong", {display, "32"});
			}
			if (!display.empty())
				obj->setDisplayName(display);
		}
		// Seed initial health score for active players
		if (criteria == ScoreObjectiveCriteria::HEALTH)
		{
			for (EntityPlayer *p : getAllPlayers(sender->getMinecraft()))
			{
				if (p != nullptr)
				{
					Score *sc = board->getValueFromObjective(p->username, obj);
					if (sc != nullptr)
						sc->updateFromCriteria({p});
				}
			}
		}
		notifyAdmins(sender, "commands.scoreboard.objectives.add.success", {name});
	}

	static void removeObjective(ICommandSender *sender, const std::string &name)
	{
		Scoreboard *board = getBoard(sender);
		ScoreObjective *obj = requireObjective(sender, name, false);
		board->removeObjective(obj);
		notifyAdmins(sender, "commands.scoreboard.objectives.remove.success", {name});
	}

	static void setDisplay(ICommandSender *sender, const std::vector<std::string> &args)
	{
		Scoreboard *board = getBoard(sender);
		const int_t slot = Scoreboard::getObjectiveDisplaySlotNumber(args[2]);
		if (slot < 0)
			throw CommandException("commands.scoreboard.objectives.setdisplay.invalidSlot", {args[2]});
		ScoreObjective *obj = (args.size() == 4) ? requireObjective(sender, args[3], false) : nullptr;
		board->setObjectiveInDisplaySlot(slot, obj);
		if (obj != nullptr)
			notifyAdmins(sender, "commands.scoreboard.objectives.setdisplay.successSet",
				{Scoreboard::getObjectiveDisplaySlot(slot), obj->getName()});
		else
			notifyAdmins(sender, "commands.scoreboard.objectives.setdisplay.successCleared",
				{Scoreboard::getObjectiveDisplaySlot(slot)});
	}

	static void listPlayers(ICommandSender *sender, const std::vector<std::string> &args)
	{
		Scoreboard *board = getBoard(sender);
		if (args.size() > 2)
		{
			const std::string target = resolveTargetName(sender, args[2]);
			const auto scores = board->getPlayerScores(target);
			if (scores.empty())
				throw CommandException("commands.scoreboard.players.list.player.empty", {target});
			sender->sendChatToPlayer(kChatDarkGreen + sender->translateString("commands.scoreboard.players.list.player.count",
				{std::to_string(scores.size()), target}));
			for (const auto &entry : scores)
			{
				Score *s = entry.second;
				sender->sendChatToPlayer(sender->translateString("commands.scoreboard.players.list.player.entry",
					{std::to_string(s->getScorePoints()), s->getObjective()->getDisplayName(), s->getObjective()->getName()}));
			}
		}
		else
		{
			const auto names = board->getObjectiveNames();
			if (names.empty())
				throw CommandException("commands.scoreboard.players.list.empty");
			sender->sendChatToPlayer(kChatDarkGreen + sender->translateString("commands.scoreboard.players.list.count",
				{std::to_string(names.size())}));
			sender->sendChatToPlayer(joinNiceString(names));
		}
	}

	static void modifyPlayerScore(ICommandSender *sender, const std::string &act, const std::vector<std::string> &args)
	{
		const std::string target = resolveTargetName(sender, args[2]);
		ScoreObjective *obj = requireObjective(sender, args[3], true);
		const int val = (act == "set") ? parseInt(sender, args[4]) : parseIntWithMin(sender, args[4], 1);
		Scoreboard *board = getBoard(sender);
		Score *score = board->getValueFromObjective(target, obj);
		if (act == "set")
			score->setScorePoints(val);
		else if (act == "add")
			score->increaseScore(val);
		else
			score->decreaseScore(val);
		notifyAdmins(sender, "commands.scoreboard.players.set.success",
			{obj->getName(), target, std::to_string(score->getScorePoints())});
	}

	static void listTeams(ICommandSender *sender, const std::vector<std::string> &args)
	{
		Scoreboard *board = getBoard(sender);
		if (args.size() > 2)
		{
			ScorePlayerTeam *team = requireTeam(sender, args[2]);
			const auto &members = team->getMembershipCollection();
			if (members.empty())
				throw CommandException("commands.scoreboard.teams.list.player.empty", {team->getRegisteredName()});
			sender->sendChatToPlayer(kChatDarkGreen + sender->translateString("commands.scoreboard.teams.list.player.count",
				{std::to_string(members.size()), team->getRegisteredName()}));
			sender->sendChatToPlayer(joinNiceString(std::vector<std::string>(members.begin(), members.end())));
		}
		else
		{
			const auto allTeams = board->getTeams();
			if (allTeams.empty())
				throw CommandException("commands.scoreboard.teams.list.empty");
			sender->sendChatToPlayer(kChatDarkGreen + sender->translateString("commands.scoreboard.teams.list.count",
				{std::to_string(allTeams.size())}));
			for (ScorePlayerTeam *team : allTeams)
			{
				sender->sendChatToPlayer(sender->translateString("commands.scoreboard.teams.list.entry",
					{team->getRegisteredName(), team->getTeamName(), std::to_string(team->getMembershipCollection().size())}));
			}
		}
	}

	static void addTeam(ICommandSender *sender, const std::vector<std::string> &args)
	{
		const std::string &name = args[2];
		Scoreboard *board = getBoard(sender);
		if (board->getTeam(name) != nullptr)
			throw CommandException("commands.scoreboard.teams.add.alreadyExists", {name});
		if (name.size() > 16)
			throw CommandException("commands.scoreboard.teams.add.tooLong", {name, "16"});
		ScorePlayerTeam *team = board->createTeam(name);
		if (args.size() > 3)
		{
			const std::string display = joinArgs(sender, args, 3, false);
			if (display.size() > 32)
			{
				board->removeTeam(team);
				throw CommandException("commands.scoreboard.teams.add.displayTooLong", {display, "32"});
			}
			if (!display.empty())
				team->setTeamName(display);
		}
		notifyAdmins(sender, "commands.scoreboard.teams.add.success", {name});
	}

	static void joinTeam(ICommandSender *sender, const std::vector<std::string> &args)
	{
		Scoreboard *board = getBoard(sender);
		ScorePlayerTeam *team = requireTeam(sender, args[2]);
		std::vector<std::string> joined;
		if (args.size() == 3 && sender->getSenderPlayer() != nullptr)
		{
			const std::string pName = sender->getSenderPlayer()->username;
			board->addPlayerToTeam(pName, team);
			joined.push_back(pName);
		}
		else
		{
			for (std::size_t i = 3; i < args.size(); ++i)
			{
				const std::string target = resolveTargetName(sender, args[i]);
				board->addPlayerToTeam(target, team);
				joined.push_back(target);
			}
		}
		if (!joined.empty())
		{
			notifyAdmins(sender, "commands.scoreboard.teams.join.success",
				{std::to_string(joined.size()), team->getRegisteredName(), joinNiceString(joined)});
		}
	}

	static void leaveTeam(ICommandSender *sender, const std::vector<std::string> &args)
	{
		Scoreboard *board = getBoard(sender);
		std::vector<std::string> ok;
		std::vector<std::string> fail;
		if (args.size() == 2 && sender->getSenderPlayer() != nullptr)
		{
			const std::string pName = sender->getSenderPlayer()->username;
			if (board->removePlayerFromTeams(pName))
				ok.push_back(pName);
			else
				fail.push_back(pName);
		}
		else
		{
			for (std::size_t i = 2; i < args.size(); ++i)
			{
				const std::string target = resolveTargetName(sender, args[i]);
				if (board->removePlayerFromTeams(target))
					ok.push_back(target);
				else
					fail.push_back(target);
			}
		}
		if (!ok.empty())
			notifyAdmins(sender, "commands.scoreboard.teams.leave.success", {std::to_string(ok.size()), joinNiceString(ok)});
		if (!fail.empty())
			throw CommandException("commands.scoreboard.teams.leave.failure", {std::to_string(fail.size()), joinNiceString(fail)});
	}

	static void setTeamOption(ICommandSender *sender, const std::vector<std::string> &args)
	{
		ScorePlayerTeam *team = requireTeam(sender, args[2]);
		const std::string opt = String::toLowerCaseJava(args[3]);
		if (opt != "color" && opt != "friendlyfire" && opt != "seefriendlyinvisibles")
			throw WrongUsageException("commands.scoreboard.teams.option.usage");
		if (args.size() == 4)
		{
			if (opt == "color")
				throw WrongUsageException("commands.scoreboard.teams.option.noValue", {opt, joinNiceString(validColorNames())});
			throw WrongUsageException("commands.scoreboard.teams.option.noValue", {opt, "true, false"});
		}
		const std::string &val = args[4];
		if (opt == "color")
		{
			const std::string code = colorCodeByName(val);
			if (code.empty())
				throw WrongUsageException("commands.scoreboard.teams.option.noValue", {opt, joinNiceString(validColorNames())});
			if (String::equalsIgnoreCaseJava(val, "reset"))
			{
				team->setNamePrefix("");
				team->setNameSuffix("");
			}
			else
			{
				team->setNamePrefix(code);
				team->setNameSuffix("\xc2\xa7" "r");
			}
		}
		else if (opt == "friendlyfire")
		{
			if (!String::equalsIgnoreCaseJava(val, "true") && !String::equalsIgnoreCaseJava(val, "false"))
				throw WrongUsageException("commands.scoreboard.teams.option.noValue", {opt, "true, false"});
			team->setAllowFriendlyFire(String::equalsIgnoreCaseJava(val, "true"));
		}
		else
		{
			if (!String::equalsIgnoreCaseJava(val, "true") && !String::equalsIgnoreCaseJava(val, "false"))
				throw WrongUsageException("commands.scoreboard.teams.option.noValue", {opt, "true, false"});
			team->setSeeFriendlyInvisiblesEnabled(String::equalsIgnoreCaseJava(val, "true"));
		}
		notifyAdmins(sender, "commands.scoreboard.teams.option.success", {opt, team->getRegisteredName(), val});
	}
};

// ─── /help (alias: ?) ────────────────────────────────────────────────────────
class CommandHelp : public CommandBase
{
public:
	explicit CommandHelp(const CommandHandler *handler) : owner(handler) {}

	std::string getCommandName() const override { return "help"; }
	std::vector<std::string> getCommandAliases() const override { return {"?"}; }
	int getRequiredPermissionLevel() const override { return 0; }
	std::string getCommandUsage(ICommandSender *sender) const override { return sender->translateString("commands.help.usage"); }

	void processCommand(ICommandSender *sender, const std::vector<std::string> &args) override
	{
		std::vector<ICommand *> cmds = owner->getPossibleCommands(sender);
		std::sort(cmds.begin(), cmds.end(), [](const ICommand *a, const ICommand *b) {
			return a->getCommandName() < b->getCommandName();
		});

		constexpr int kPerPage = 7;
		const int maxPageIndex = cmds.empty() ? 0 : static_cast<int>((cmds.size() - 1) / kPerPage);
		int pageIndex = 0;

		if (!args.empty())
		{
			try
			{
				pageIndex = parseIntBounded(sender, args[0], 1, maxPageIndex + 1) - 1;
			}
			catch (const NumberInvalidException &)
			{
				const auto &map = owner->getCommands();
				auto it = map.find(args[0]);
				if (it != map.end() && it->second != nullptr)
					throw WrongUsageException(it->second->getCommandUsage(sender));
				throw CommandNotFoundException();
			}
		}

		const int endIndex = std::min<int>((pageIndex + 1) * kPerPage, static_cast<int>(cmds.size()));
		sender->sendChatToPlayer(kChatDarkGreen + sender->translateString("commands.help.header",
			{std::to_string(pageIndex + 1), std::to_string(maxPageIndex + 1)}));

		for (int i = pageIndex * kPerPage; i < endIndex; ++i)
			sender->sendChatToPlayer(cmds[static_cast<std::size_t>(i)]->getCommandUsage(sender));

		if (pageIndex == 0 && sender->getSenderPlayer() != nullptr)
			sender->sendChatToPlayer(kChatGreen + sender->translateString("commands.help.footer"));
	}

private:
	const CommandHandler *owner;
};
} // namespace

ServerCommandManager &ServerCommandManager::getInstance()
{
	static ServerCommandManager s_instance;
	return s_instance;
}

ServerCommandManager::ServerCommandManager()
{
	registerCommand(std::make_unique<CommandTime>());
	registerCommand(std::make_unique<CommandGameMode>());
	registerCommand(std::make_unique<CommandDifficulty>());
	registerCommand(std::make_unique<CommandDefaultGameMode>());
	registerCommand(std::make_unique<CommandKill>());
	registerCommand(std::make_unique<CommandToggleDownfall>());
	registerCommand(std::make_unique<CommandWeather>());
	registerCommand(std::make_unique<CommandXP>());
	registerCommand(std::make_unique<CommandServerTp>());
	registerCommand(std::make_unique<CommandGive>());
	registerCommand(std::make_unique<CommandEnchant>());
	registerCommand(std::make_unique<CommandServerEmote>());
	registerCommand(std::make_unique<CommandShowSeed>());
	registerCommand(std::make_unique<CommandHelp>(this));
	registerCommand(std::make_unique<CommandServerMessage>());
	registerCommand(std::make_unique<CommandServerSay>());
	registerCommand(std::make_unique<CommandSetSpawnpoint>());
	registerCommand(std::make_unique<CommandGameRule>());
	registerCommand(std::make_unique<CommandClearInventory>());
	registerCommand(std::make_unique<CommandScoreboard>());
	CommandBase::setAdminCommander(this);
}

void ServerCommandManager::notifyAdmins(ICommandSender *sender, int flags, const std::string &translationKey, const std::vector<std::string> &args)
{
	if (sender == nullptr)
		return;
	Minecraft *mc = sender->getMinecraft();
	bool broadcast = true;
	if (sender->isCommandBlock() && mc != nullptr && mc->theWorld != nullptr &&
	    !mc->theWorld->getGameRules().getGameRuleBooleanValue("commandBlockOutput"))
	{
		broadcast = false;
	}

	const std::string translated = sender->translateString(translationKey, args);
	MC_LOG_INFO("cmd", "[%s: %s]\n", sender->getCommandSenderName().c_str(), translated.c_str());

	// Java 1.5.2: bit 0 of flags suppresses direct echo ONLY when server broadcasts separately;
	// in singleplayer, the executing player always sees the command result (unless silenced command block).
	if (broadcast && ((flags & 1) != 1 || sender->getSenderPlayer() != nullptr))
		sender->sendChatToPlayer(translated);
}
