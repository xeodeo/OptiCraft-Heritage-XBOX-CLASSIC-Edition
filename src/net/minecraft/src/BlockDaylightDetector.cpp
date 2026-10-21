#include "BlockDaylightDetector.h"
#include "EnumSkyBlock.h"
#include "IBlockAccess.h"
#include "Material.h"
#include "MathHelper.h"
#include "Texture152Atlas.h"
#include "TileEntityDaylightDetector.h"
#include "World.h"
#include "WorldProvider.h"
#include <cmath>

BlockDaylightDetector::BlockDaylightDetector(int_t id)
	: BlockContainer(id, Material::wood)
{
	topTextureIndex = TEX152_BLOCK_ICON("daylightDetector_top.png");
	sideTextureIndex = TEX152_BLOCK_ICON("daylightDetector_side.png");
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.375f, 1.0f);
}

void BlockDaylightDetector::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z)
{
	(void)iblockaccess; (void)x; (void)y; (void)z;
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.375f, 1.0f);
}

int_t BlockDaylightDetector::isProvidingWeakPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	(void)side;
	return iblockaccess->getBlockMetadata(x, y, z);
}

int_t BlockDaylightDetector::isProvidingStrongPower(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	(void)iblockaccess; (void)x; (void)y; (void)z; (void)side;
	return 0;
}

bool BlockDaylightDetector::isPoweringTo(IBlockAccess *iblockaccess, int_t x, int_t y, int_t z, int_t side)
{
	return isProvidingWeakPower(iblockaccess, x, y, z, side) > 0;
}

bool BlockDaylightDetector::isIndirectlyPoweringTo(World *world, int_t x, int_t y, int_t z, int_t side)
{
	(void)world; (void)x; (void)y; (void)z; (void)side;
	return false;
}

bool BlockDaylightDetector::canProvidePower()
{
	return true;
}

bool BlockDaylightDetector::renderAsNormalBlock()
{
	return false;
}

bool BlockDaylightDetector::isOpaqueCube()
{
	return false;
}

int_t BlockDaylightDetector::getRenderType()
{
	return 0;
}

int_t BlockDaylightDetector::getBlockTextureFromSideAndMetadata(int_t side, int_t meta)
{
	(void)meta;
	return side == 1 ? topTextureIndex : sideTextureIndex;
}

TileEntity *BlockDaylightDetector::getBlockEntity()
{
	return createNewTileEntity(nullptr);
}

TileEntity *BlockDaylightDetector::createNewTileEntity(World *world)
{
	(void)world;
	return new TileEntityDaylightDetector();
}

void BlockDaylightDetector::updateLightLevel(World *world, int_t x, int_t y, int_t z)
{
	if (world->worldProvider != nullptr && !world->worldProvider->hasNoSky)
	{
		int_t current = world->getBlockMetadata(x, y, z);
		int_t light = world->getSavedLightValue(EnumSkyBlock::Sky, x, y, z) - world->skylightSubtracted;
		float angle = world->getCelestialAngleRadians(1.0f);
		if (angle < 3.14159265358979323846f)
		{
			angle += (0.0f - angle) * 0.2f;
		}
		else
		{
			angle += (6.28318530717958647692f - angle) * 0.2f;
		}

		light = (int_t)std::round((float)light * MathHelper::cos(angle));
		if (light < 0) light = 0;
		if (light > 15) light = 15;
		if (current != light)
		{
			world->setBlockMetadataWithNotify(x, y, z, light);
		}
	}
}
