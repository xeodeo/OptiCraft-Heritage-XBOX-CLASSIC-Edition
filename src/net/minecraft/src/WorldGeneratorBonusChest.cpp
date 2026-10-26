#include "WorldGeneratorBonusChest.h"

#include "Block.h"
#include "BlockLeaves.h"
#include "TileEntityChest.h"
#include "World.h"
#include "java/Random.h"

WorldGeneratorBonusChest::WorldGeneratorBonusChest(const WeightedRandomChestContent *loot, std::size_t count, int_t rolls)
	: bonusChestContent(loot),
	  bonusChestContentCount(count),
	  itemsToGenerateInBonusChest(rolls)
{
}

bool WorldGeneratorBonusChest::generate(World *world, Random &random, int_t x, int_t y, int_t z)
{
	int_t blockId = 0;
	const int_t leavesId = Block::leaves != nullptr ? Block::leaves->blockID : 18;
	const int_t chestId = Block::chest != nullptr ? Block::chest->blockID : 54;
	const int_t torchId = Block::torchWood != nullptr ? Block::torchWood->blockID : 50;

	while (((blockId = world->getBlockId(x, y, z)) == 0 || blockId == leavesId) && y > 1)
	{
		--y;
	}

	if (y < 1)
		return false;

	++y;

	for (int_t attempt = 0; attempt < 4; ++attempt)
	{
		const int_t targetX = x + random.nextInt(4) - random.nextInt(4);
		const int_t targetY = y + random.nextInt(3) - random.nextInt(3);
		const int_t targetZ = z + random.nextInt(4) - random.nextInt(4);

		if (world->isAirBlock(targetX, targetY, targetZ) && world->doesBlockHaveSolidTopSurface(targetX, targetY - 1, targetZ))
		{
			world->setBlock(targetX, targetY, targetZ, chestId, 0, 2);
			TileEntityChest *chest = dynamic_cast<TileEntityChest *>(world->getBlockTileEntity(targetX, targetY, targetZ));
			if (chest != nullptr)
			{
				WeightedRandomChestContent::generateChestContents(random, bonusChestContent, bonusChestContentCount, chest, itemsToGenerateInBonusChest);
			}

			if (world->isAirBlock(targetX - 1, targetY, targetZ) && world->doesBlockHaveSolidTopSurface(targetX - 1, targetY - 1, targetZ))
			{
				world->setBlock(targetX - 1, targetY, targetZ, torchId, 0, 2);
			}
			if (world->isAirBlock(targetX + 1, targetY, targetZ) && world->doesBlockHaveSolidTopSurface(targetX + 1, targetY - 1, targetZ))
			{
				world->setBlock(targetX + 1, targetY, targetZ, torchId, 0, 2);
			}
			if (world->isAirBlock(targetX, targetY, targetZ - 1) && world->doesBlockHaveSolidTopSurface(targetX, targetY - 1, targetZ - 1))
			{
				world->setBlock(targetX, targetY, targetZ - 1, torchId, 0, 2);
			}
			if (world->isAirBlock(targetX, targetY, targetZ + 1) && world->doesBlockHaveSolidTopSurface(targetX, targetY - 1, targetZ + 1))
			{
				world->setBlock(targetX, targetY, targetZ + 1, torchId, 0, 2);
			}

			return true;
		}
	}

	return false;
}
