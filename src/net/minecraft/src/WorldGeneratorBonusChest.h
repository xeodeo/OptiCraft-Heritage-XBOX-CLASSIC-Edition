#pragma once

#include "WorldGenerator.h"
#include "WeightedRandomChestContent.h"

// net.minecraft.src.WorldGeneratorBonusChest
class WorldGeneratorBonusChest : public WorldGenerator
{
public:
	WorldGeneratorBonusChest(const WeightedRandomChestContent *loot, std::size_t count, int_t rolls);
	~WorldGeneratorBonusChest() override = default;

	bool generate(World *world, Random &random, int_t x, int_t y, int_t z) override;

private:
	const WeightedRandomChestContent *bonusChestContent;
	std::size_t bonusChestContentCount;
	int_t itemsToGenerateInBonusChest;
};
