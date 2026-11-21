#include "BlockIce.h"
#include "Material.h"
#include "World.h"
#include "WorldProvider.h"
#include "Block.h"
#include "EnchantmentHelper.h"
#include "EntityPlayer.h"
#include "EnumSkyBlock.h"
#include "IBlockAccess.h"
#include "ItemStack.h"
#include "StatList.h"
#include "Config.h"

BlockIce::BlockIce(int_t i, int_t j) : BlockBreakable(i, j, Material::ice, false)
{
	slipperiness = 0.98f;
	setTickOnLoad(true);
}

int_t BlockIce::getRenderBlockPass() { return Config::isOpaqueIce() ? 0 : 1; }

bool BlockIce::isOpaqueCube()
{
	return Config::isOpaqueIce();
}

bool BlockIce::shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	return BlockBreakable::shouldSideBeRendered(iblockaccess, i, j, k, 1 - l);
}

void BlockIce::harvestBlock(World *world, EntityPlayer *entityplayer, int_t i, int_t j, int_t k, int_t l)
{
	if (world == nullptr || entityplayer == nullptr)
		return;

	entityplayer->addStat(StatList::mineBlockStatArray[blockID], 1);
	entityplayer->addExhaustion(0.025f);
	if (func_50074_q() && EnchantmentHelper::getSilkTouchModifier(entityplayer->inventory))
	{
		ItemStack *stack = createStackedBlock(l);
		if (stack != nullptr)
			dropBlockAsItem_do(world, i, j, k, stack);
	}
	else
	{
		if (world->worldProvider != nullptr && world->worldProvider->isHellWorld)
		{
			world->setBlockWithNotify(i, j, k, 0);
			return;
		}

		const int_t fortune = EnchantmentHelper::getFortuneModifier(entityplayer->inventory);
		dropBlockAsItem(world, i, j, k, l, fortune);
		Material *material = world->getBlockMaterial(i, j - 1, k);
		if (material->getIsSolid() || material->getIsLiquid())
		{
			world->setBlockWithNotify(i, j, k, Block::waterMoving->blockID);
		}
	}
}

int_t BlockIce::quantityDropped(Random &random) { return 0; }

void BlockIce::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
	(void)random;
	if (world->getSavedLightValue(EnumSkyBlock::Block, i, j, k) > 11 - Block::lightOpacity[blockID])
	{
		if (world->worldProvider != nullptr && world->worldProvider->isHellWorld)
		{
			world->setBlockWithNotify(i, j, k, 0);
			return;
		}

		dropBlockAsItem(world, i, j, k, world->getBlockMetadata(i, j, k), 0);
		world->setBlockWithNotify(i, j, k, Block::waterStill->blockID);
	}
}

int_t BlockIce::getMobilityFlag() { return 0; }
