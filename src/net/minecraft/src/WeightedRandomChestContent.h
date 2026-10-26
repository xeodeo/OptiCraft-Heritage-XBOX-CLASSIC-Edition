#pragma once

#include "WeightedRandomChoice.h"
#include <cstddef>
#include <vector>

class Random;
class IInventory;
class TileEntityDispenser;
class ItemStack;

// net.minecraft.src.WeightedRandomChestContent
class WeightedRandomChestContent : public WeightedRandomChoice
{
public:
	WeightedRandomChestContent(int_t itemId, int_t itemMetadata, int_t minChance, int_t maxChance, int_t weight);
	WeightedRandomChestContent(const ItemStack &itemStack, int_t minChance, int_t maxChance, int_t weight);

	int_t theItemId;
	int_t theItemDamage;
	int_t theMinimumChanceToGenerateItem;
	int_t theMaximumChanceToGenerateItem;

	static void generateChestContents(Random &random, const WeightedRandomChestContent *loot, std::size_t lootCount, IInventory *inv, int_t rolls);
	static void generateChestContents(Random &random, const std::vector<WeightedRandomChestContent> &loot, IInventory *inv, int_t rolls);
	static void generateDispenserContents(Random &random, const WeightedRandomChestContent *loot, std::size_t lootCount, TileEntityDispenser *dispenser, int_t rolls);
	static void generateDispenserContents(Random &random, const std::vector<WeightedRandomChestContent> &loot, TileEntityDispenser *dispenser, int_t rolls);
};
