#include "BlockFlowerPot.h"
#include "BlockFlower.h"
#include "BlockDeadBush.h"
#include "BlockTallGrass.h"
#include "Texture152Atlas.h"

#include "Block.h"
#include "EntityPlayer.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemStack.h"
#include "Material.h"
#include "World.h"

BlockFlowerPot::BlockFlowerPot(int_t id)
	: Block(id, TEX152_BLOCK_ICON("flowerPot.png"), Material::circuits) // 1.5.2 icon: flowerPot.png
{
	setBlockBoundsForItemRender();
}

void BlockFlowerPot::setBlockBoundsForItemRender()
{
	const float size = 0.375f;
	const float half = size / 2.0f;
	setBlockBounds(0.5f - half, 0.0f, 0.5f - half, 0.5f + half, size, 0.5f + half);
}

bool BlockFlowerPot::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	ItemStack *current = entityplayer->inventory->getCurrentItem();
	if (current == nullptr)
	{
		return false;
	}
	if (world->getBlockMetadata(i, j, k) != 0)
	{
		return false;
	}

	const int_t meta = getMetaForPlant(current);
	if (meta > 0)
	{
		world->setBlockMetadataWithNotify(i, j, k, meta);
		if (!entityplayer->capabilities.isCreativeMode)
		{
			--current->stackSize;
			if (current->stackSize <= 0)
			{
				entityplayer->inventory->setInventorySlotContents(entityplayer->inventory->currentItem, nullptr);
			}
		}
		return true;
	}
	return false;
}

int_t BlockFlowerPot::idPicked(World *world, int_t i, int_t j, int_t k)
{
	ItemStack *plant = getPlantForMeta(world->getBlockMetadata(i, j, k));
	if (plant != nullptr)
	{
		int_t id = plant->itemID;
		delete plant;
		return id;
	}
	return Item::flowerPot != nullptr ? Item::flowerPot->shiftedIndex : 390;
}

int_t BlockFlowerPot::getDamageValue(World *world, int_t i, int_t j, int_t k)
{
	ItemStack *plant = getPlantForMeta(world->getBlockMetadata(i, j, k));
	if (plant != nullptr)
	{
		int_t dmg = plant->getItemDamage();
		delete plant;
		return dmg;
	}
	return Item::flowerPot != nullptr ? Item::flowerPot->shiftedIndex : 390;
}

bool BlockFlowerPot::canPlaceBlockAt(World *world, int_t i, int_t j, int_t k)
{
	return Block::canPlaceBlockAt(world, i, j, k) && world->isBlockNormalCube(i, j - 1, k);
}

void BlockFlowerPot::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	if (!world->isBlockNormalCube(i, j - 1, k))
	{
		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k), 0);
		world->setBlockWithNotify(i, j, k, 0);
	}
}

void BlockFlowerPot::dropBlockAsItemWithChance(World *world, int_t i, int_t j, int_t k, int_t metadata, float chance, int_t fortune)
{
	Block::dropBlockAsItemWithChance(world, i, j, k, metadata, chance, fortune);
	if (metadata > 0)
	{
		ItemStack *plant = getPlantForMeta(metadata);
		if (plant != nullptr)
		{
			dropBlockAsItem_do(world, i, j, k, plant);
		}
	}
}

int_t BlockFlowerPot::idDropped(int_t metadata, Random &random)
{
	(void)metadata;
	(void)random;
	return Item::flowerPot != nullptr ? Item::flowerPot->shiftedIndex : 390;
}

ItemStack *BlockFlowerPot::getPlantForMeta(int_t meta)
{
	switch (meta)
	{
	case 1:  return Block::plantRed != nullptr ? new ItemStack(Block::plantRed) : nullptr;
	case 2:  return Block::plantYellow != nullptr ? new ItemStack(Block::plantYellow) : nullptr;
	case 3:  return Block::sapling != nullptr ? new ItemStack(Block::sapling, 1, 0) : nullptr;
	case 4:  return Block::sapling != nullptr ? new ItemStack(Block::sapling, 1, 1) : nullptr;
	case 5:  return Block::sapling != nullptr ? new ItemStack(Block::sapling, 1, 2) : nullptr;
	case 6:  return Block::sapling != nullptr ? new ItemStack(Block::sapling, 1, 3) : nullptr;
	case 7:  return Block::mushroomRed != nullptr ? new ItemStack(Block::mushroomRed) : nullptr;
	case 8:  return Block::mushroomBrown != nullptr ? new ItemStack(Block::mushroomBrown) : nullptr;
	case 9:  return Block::cactus != nullptr ? new ItemStack(Block::cactus) : nullptr;
	case 10: return Block::deadBush != nullptr ? new ItemStack(Block::deadBush) : nullptr;
	case 11: return Block::tallGrass != nullptr ? new ItemStack(Block::tallGrass, 1, 2) : nullptr;
	default: return nullptr;
	}
}

int_t BlockFlowerPot::getMetaForPlant(ItemStack *stack)
{
	if (stack == nullptr)
	{
		return 0;
	}
	const int_t id = stack->itemID;
	if (Block::plantRed != nullptr && id == Block::plantRed->blockID) return 1;
	if (Block::plantYellow != nullptr && id == Block::plantYellow->blockID) return 2;
	if (Block::cactus != nullptr && id == Block::cactus->blockID) return 9;
	if (Block::mushroomBrown != nullptr && id == Block::mushroomBrown->blockID) return 8;
	if (Block::mushroomRed != nullptr && id == Block::mushroomRed->blockID) return 7;
	if (Block::deadBush != nullptr && id == Block::deadBush->blockID) return 10;
	if (Block::sapling != nullptr && id == Block::sapling->blockID)
	{
		switch (stack->getItemDamage())
		{
		case 0: return 3;
		case 1: return 4;
		case 2: return 5;
		case 3: return 6;
		default: return 0;
		}
	}
	if (Block::tallGrass != nullptr && id == Block::tallGrass->blockID && stack->getItemDamage() == 2)
	{
		return 11;
	}
	return 0;
}
