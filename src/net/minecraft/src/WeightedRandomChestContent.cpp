#include "WeightedRandomChestContent.h"

#include "IInventory.h"
#include "Item.h"
#include "ItemStack.h"
#include "TileEntityDispenser.h"
#include "java/Random.h"

WeightedRandomChestContent::WeightedRandomChestContent(
	int_t itemId, int_t itemMetadata, int_t minChance, int_t maxChance, int_t weight)
	: WeightedRandomChoice(weight),
	  theItemId(itemId),
	  theItemDamage(itemMetadata),
	  theMinimumChanceToGenerateItem(minChance),
	  theMaximumChanceToGenerateItem(maxChance)
{
}

WeightedRandomChestContent::WeightedRandomChestContent(
	const ItemStack &itemStack, int_t minChance, int_t maxChance, int_t weight)
	: WeightedRandomChoice(weight),
	  theItemId(itemStack.itemID),
	  theItemDamage(const_cast<ItemStack &>(itemStack).getItemDamage()),
	  theMinimumChanceToGenerateItem(minChance),
	  theMaximumChanceToGenerateItem(maxChance)
{
}

void WeightedRandomChestContent::generateChestContents(
	Random &random, const WeightedRandomChestContent *loot, std::size_t lootCount, IInventory *inv, int_t rolls)
{
	if (inv == nullptr || loot == nullptr || lootCount == 0 || rolls <= 0)
		return;

	int_t totalWeight = 0;
	for (std::size_t i = 0; i < lootCount; ++i)
		totalWeight += loot[i].getItemWeight();
	if (totalWeight <= 0)
		return;

	const int_t invSize = inv->getSizeInventory();
	if (invSize <= 0)
		return;

	for (int_t roll = 0; roll < rolls; ++roll)
	{
		int_t pick = random.nextInt(totalWeight);
		const WeightedRandomChestContent *chosen = nullptr;
		for (std::size_t i = 0; i < lootCount; ++i)
		{
			pick -= loot[i].getItemWeight();
			if (pick < 0)
			{
				chosen = &loot[i];
				break;
			}
		}
		if (chosen == nullptr)
			chosen = &loot[0];

		const int_t minCount = chosen->theMinimumChanceToGenerateItem;
		const int_t maxCount = chosen->theMaximumChanceToGenerateItem;
		const int_t count = minCount + (maxCount > minCount ? random.nextInt(maxCount - minCount + 1) : 0);
		if (count <= 0)
			continue;

		const int_t itemId = chosen->theItemId;
		const int_t itemDamage = chosen->theItemDamage;
		int_t maxStack = 64;
		if (itemId >= 0 && itemId < Item::ITEM_LIST_SIZE && Item::itemsList[itemId] != nullptr)
			maxStack = Item::itemsList[itemId]->getItemStackLimit();

		if (maxStack >= count)
		{
			inv->setInventorySlotContents(random.nextInt(invSize), new ItemStack(itemId, count, itemDamage));
		}
		else
		{
			for (int_t j = 0; j < count; ++j)
			{
				inv->setInventorySlotContents(random.nextInt(invSize), new ItemStack(itemId, 1, itemDamage));
			}
		}
	}
}

void WeightedRandomChestContent::generateChestContents(
	Random &random, const std::vector<WeightedRandomChestContent> &loot, IInventory *inv, int_t rolls)
{
	if (!loot.empty())
		generateChestContents(random, loot.data(), loot.size(), inv, rolls);
}

void WeightedRandomChestContent::generateDispenserContents(
	Random &random, const WeightedRandomChestContent *loot, std::size_t lootCount, TileEntityDispenser *dispenser, int_t rolls)
{
	generateChestContents(random, loot, lootCount, dispenser, rolls);
}

void WeightedRandomChestContent::generateDispenserContents(
	Random &random, const std::vector<WeightedRandomChestContent> &loot, TileEntityDispenser *dispenser, int_t rolls)
{
	if (!loot.empty())
		generateChestContents(random, loot.data(), loot.size(), dispenser, rolls);
}
