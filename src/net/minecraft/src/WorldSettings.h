#pragma once

#include "java/Type.h"
#include "java/String.h"

class WorldType;

// net.minecraft.src.WorldSettings
class WorldSettings
{
public:
	WorldSettings(int64_t seed, int_t gameType, bool mapFeaturesEnabled,
	              bool hardcoreEnabled, WorldType *terrainType, int_t worldSizeType = 0);

	int64_t getSeed() const;
	int_t getGameType() const;
	bool getHardcoreEnabled() const;
	bool isMapFeaturesEnabled() const;
	WorldType *getTerrainType() const;
	bool isLimitedWorld() const;
	int_t getWorldSizeType() const;

	WorldSettings *enableBonusChest();
	bool isBonusChestEnabled() const;
	WorldSettings *enableCommands();
	bool areCommandsAllowed() const;
	WorldSettings *func_82750_a(const jstring &options);
	WorldSettings *setGeneratorOptions(const jstring &options) { return func_82750_a(options); }
	const jstring &getGeneratorOptions() const;

private:
	int64_t seed;
	int_t gameType;
	bool mapFeaturesEnabled;
	bool hardcoreEnabled;
	WorldType *terrainType;
	int_t worldSizeType;
	bool bonusChestEnabled{false};
	bool commandsAllowed{false};
	jstring field_82751_h;
};

