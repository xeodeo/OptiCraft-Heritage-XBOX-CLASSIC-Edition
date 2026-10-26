#include "ComponentScatteredFeatureDesertPyramid.h"

#include "Block.h"
#include "Direction.h"
#include "Item.h"
#include "StructureBoundingBox.h"
#include "WeightedRandomChestContent.h"
#include "World.h"
#include "java/Random.h"

ComponentScatteredFeatureDesertPyramid::ComponentScatteredFeatureDesertPyramid(Random &random, int_t x, int_t z)
	: ComponentScatteredFeature(random, x, 64, z, 21, 15, 21),
	  chestsPlaced{false, false, false, false}
{
}

bool ComponentScatteredFeatureDesertPyramid::addComponentParts(World *world, Random &random, const StructureBoundingBox &chunkBounds)
{
	static const WeightedRandomChestContent itemsToGenerateInTemple[] = {
		WeightedRandomChestContent(Item::diamond != nullptr ? Item::diamond->itemID : 264, 0, 1, 3, 3),
		WeightedRandomChestContent(Item::ingotIron != nullptr ? Item::ingotIron->itemID : 265, 0, 1, 5, 10),
		WeightedRandomChestContent(Item::ingotGold != nullptr ? Item::ingotGold->itemID : 266, 0, 2, 7, 15),
		WeightedRandomChestContent(Item::emerald != nullptr ? Item::emerald->itemID : 388, 0, 1, 3, 2),
		WeightedRandomChestContent(Item::bone != nullptr ? Item::bone->itemID : 352, 0, 4, 6, 20),
		WeightedRandomChestContent(Item::rottenFlesh != nullptr ? Item::rottenFlesh->itemID : 367, 0, 3, 7, 16)
	};
	constexpr std::size_t templeLootCount = sizeof(itemsToGenerateInTemple) / sizeof(itemsToGenerateInTemple[0]);

	const int_t ssId = Block::sandStone != nullptr ? Block::sandStone->blockID : 24;
	const int_t stairsSsId = Block::stairsSandStone != nullptr ? Block::stairsSandStone->blockID : 128;
	const int_t slabId = Block::stoneSingleSlab != nullptr ? Block::stoneSingleSlab->blockID : 44;
	const int_t clothId = Block::cloth != nullptr ? Block::cloth->blockID : 35;
	const int_t plateId = Block::pressurePlateStone != nullptr ? Block::pressurePlateStone->blockID : 70;
	const int_t tntId = Block::tnt != nullptr ? Block::tnt->blockID : 46;

	fillWithBlocks(world, chunkBounds, 0, -4, 0, scatteredFeatureSizeX - 1, 0, scatteredFeatureSizeZ - 1, ssId, ssId, false);

	for (int_t pos = 1; pos <= 9; ++pos)
	{
		fillWithBlocks(world, chunkBounds, pos, pos, pos, scatteredFeatureSizeX - 1 - pos, pos, scatteredFeatureSizeZ - 1 - pos, ssId, ssId, false);
		fillWithBlocks(world, chunkBounds, pos + 1, pos, pos + 1, scatteredFeatureSizeX - 2 - pos, pos, scatteredFeatureSizeZ - 2 - pos, 0, 0, false);
	}

	for (int_t x = 0; x < scatteredFeatureSizeX; ++x)
	{
		for (int_t z = 0; z < scatteredFeatureSizeZ; ++z)
		{
			fillCurrentPositionBlocksDownwards(world, ssId, 0, x, -5, z, chunkBounds);
		}
	}

	const int_t stairsNorth = getMetadataWithOffset(stairsSsId, 3);
	const int_t stairsSouth = getMetadataWithOffset(stairsSsId, 2);
	const int_t stairsEast = getMetadataWithOffset(stairsSsId, 0);
	const int_t stairsWest = getMetadataWithOffset(stairsSsId, 1);
	const int_t orangeWool = 1;
	const int_t blueWool = 11;

	// Towers
	fillWithBlocks(world, chunkBounds, 0, 0, 0, 4, 9, 4, ssId, 0, false);
	fillWithBlocks(world, chunkBounds, 1, 10, 1, 3, 10, 3, ssId, ssId, false);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsNorth, 2, 10, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsSouth, 2, 10, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsEast, 0, 10, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsWest, 4, 10, 2, chunkBounds);

	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 5, 0, 0, scatteredFeatureSizeX - 1, 9, 4, ssId, 0, false);
	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 4, 10, 1, scatteredFeatureSizeX - 2, 10, 3, ssId, ssId, false);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsNorth, scatteredFeatureSizeX - 3, 10, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsSouth, scatteredFeatureSizeX - 3, 10, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsEast, scatteredFeatureSizeX - 5, 10, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsWest, scatteredFeatureSizeX - 1, 10, 2, chunkBounds);

	// Entrance
	fillWithBlocks(world, chunkBounds, 8, 0, 0, 12, 4, 4, ssId, 0, false);
	fillWithBlocks(world, chunkBounds, 9, 1, 0, 11, 3, 4, 0, 0, false);
	placeBlockAtCurrentPosition(world, ssId, 2, 9, 1, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 9, 2, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 9, 3, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 10, 3, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 11, 3, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 11, 2, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 11, 1, 1, chunkBounds);

	fillWithBlocks(world, chunkBounds, 4, 1, 1, 8, 3, 3, ssId, 0, false);
	fillWithBlocks(world, chunkBounds, 4, 1, 2, 8, 2, 2, 0, 0, false);
	fillWithBlocks(world, chunkBounds, 12, 1, 1, 16, 3, 3, ssId, 0, false);
	fillWithBlocks(world, chunkBounds, 12, 1, 2, 16, 2, 2, 0, 0, false);

	fillWithBlocks(world, chunkBounds, 5, 4, 5, scatteredFeatureSizeX - 6, 4, scatteredFeatureSizeZ - 6, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, 9, 4, 9, 11, 4, 11, 0, 0, false);
	fillWithMetadataBlocks(world, chunkBounds, 8, 1, 8, 8, 3, 8, ssId, 2, ssId, 2, false);
	fillWithMetadataBlocks(world, chunkBounds, 12, 1, 8, 12, 3, 8, ssId, 2, ssId, 2, false);
	fillWithMetadataBlocks(world, chunkBounds, 8, 1, 12, 8, 3, 12, ssId, 2, ssId, 2, false);
	fillWithMetadataBlocks(world, chunkBounds, 12, 1, 12, 12, 3, 12, ssId, 2, ssId, 2, false);

	fillWithBlocks(world, chunkBounds, 1, 1, 5, 4, 4, 11, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 5, 1, 5, scatteredFeatureSizeX - 2, 4, 11, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, 6, 7, 9, 6, 7, 11, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 7, 7, 9, scatteredFeatureSizeX - 7, 7, 11, ssId, ssId, false);
	fillWithMetadataBlocks(world, chunkBounds, 5, 5, 9, 5, 7, 11, ssId, 2, ssId, 2, false);
	fillWithMetadataBlocks(world, chunkBounds, scatteredFeatureSizeX - 6, 5, 9, scatteredFeatureSizeX - 6, 7, 11, ssId, 2, ssId, 2, false);

	placeBlockAtCurrentPosition(world, 0, 0, 5, 5, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 5, 6, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 6, 6, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, scatteredFeatureSizeX - 6, 5, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, scatteredFeatureSizeX - 6, 6, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, scatteredFeatureSizeX - 7, 6, 10, chunkBounds);

	fillWithBlocks(world, chunkBounds, 2, 4, 4, 2, 6, 4, 0, 0, false);
	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 3, 4, 4, scatteredFeatureSizeX - 3, 6, 4, 0, 0, false);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsNorth, 2, 4, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsNorth, 2, 3, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsNorth, scatteredFeatureSizeX - 3, 4, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsNorth, scatteredFeatureSizeX - 3, 3, 4, chunkBounds);

	fillWithBlocks(world, chunkBounds, 1, 1, 3, 2, 2, 3, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 3, 1, 3, scatteredFeatureSizeX - 2, 2, 3, ssId, ssId, false);
	placeBlockAtCurrentPosition(world, stairsSsId, 0, 1, 1, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, 0, scatteredFeatureSizeX - 2, 1, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, slabId, 1, 1, 2, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, slabId, 1, scatteredFeatureSizeX - 2, 2, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsWest, 2, 1, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsSsId, stairsEast, scatteredFeatureSizeX - 3, 1, 2, chunkBounds);

	fillWithBlocks(world, chunkBounds, 4, 3, 5, 4, 3, 18, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 5, 3, 5, scatteredFeatureSizeX - 5, 3, 17, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, 3, 1, 5, 4, 2, 16, 0, 0, false);
	fillWithBlocks(world, chunkBounds, scatteredFeatureSizeX - 6, 1, 5, scatteredFeatureSizeX - 5, 2, 16, 0, 0, false);

	for (int_t z = 5; z <= 17; z += 2)
	{
		placeBlockAtCurrentPosition(world, ssId, 2, 4, 1, z, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 1, 4, 2, z, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, scatteredFeatureSizeX - 5, 1, z, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 1, scatteredFeatureSizeX - 5, 2, z, chunkBounds);
	}

	// Floor pattern
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 10, 0, 7, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 10, 0, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 9, 0, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 11, 0, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 8, 0, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 12, 0, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 7, 0, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 13, 0, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 9, 0, 11, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 11, 0, 11, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 10, 0, 12, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 10, 0, 13, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, blueWool, 10, 0, 10, chunkBounds);

	for (int_t x = 0; x <= scatteredFeatureSizeX - 1; x += scatteredFeatureSizeX - 1)
	{
		placeBlockAtCurrentPosition(world, ssId, 2, x, 2, 1, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 2, 2, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 2, 3, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 3, 1, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 3, 2, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 3, 3, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 4, 1, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 1, x, 4, 2, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 4, 3, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 5, 1, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 5, 2, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 5, 3, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 6, 1, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 1, x, 6, 2, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 6, 3, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 7, 1, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 7, 2, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 7, 3, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 8, 1, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 8, 2, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 8, 3, chunkBounds);
	}

	for (int_t x = 2; x <= scatteredFeatureSizeX - 3; x += scatteredFeatureSizeX - 3 - 2)
	{
		placeBlockAtCurrentPosition(world, ssId, 2, x - 1, 2, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 2, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x + 1, 2, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x - 1, 3, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 3, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x + 1, 3, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x - 1, 4, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 1, x, 4, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x + 1, 4, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x - 1, 5, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 5, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x + 1, 5, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x - 1, 6, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 1, x, 6, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x + 1, 6, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x - 1, 7, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x, 7, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, clothId, orangeWool, x + 1, 7, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x - 1, 8, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x, 8, 0, chunkBounds);
		placeBlockAtCurrentPosition(world, ssId, 2, x + 1, 8, 0, chunkBounds);
	}

	fillWithMetadataBlocks(world, chunkBounds, 8, 4, 0, 12, 6, 0, ssId, 2, ssId, 2, false);
	placeBlockAtCurrentPosition(world, 0, 0, 8, 6, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 12, 6, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 9, 5, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 1, 10, 5, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, clothId, orangeWool, 11, 5, 0, chunkBounds);

	// Hidden chamber & TNT trap
	fillWithMetadataBlocks(world, chunkBounds, 8, -14, 8, 12, -11, 12, ssId, 2, ssId, 2, false);
	fillWithMetadataBlocks(world, chunkBounds, 8, -10, 8, 12, -10, 12, ssId, 1, ssId, 1, false);
	fillWithMetadataBlocks(world, chunkBounds, 8, -9, 8, 12, -9, 12, ssId, 2, ssId, 2, false);
	fillWithBlocks(world, chunkBounds, 8, -8, 8, 12, -1, 12, ssId, ssId, false);
	fillWithBlocks(world, chunkBounds, 9, -11, 9, 11, -1, 11, 0, 0, false);
	placeBlockAtCurrentPosition(world, plateId, 0, 10, -11, 10, chunkBounds);
	fillWithBlocks(world, chunkBounds, 9, -13, 9, 11, -13, 11, tntId, 0, false);

	placeBlockAtCurrentPosition(world, 0, 0, 8, -11, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 8, -10, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 1, 7, -10, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 7, -11, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 12, -11, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 12, -10, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 1, 13, -10, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 13, -11, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 10, -11, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 10, -10, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 1, 10, -10, 7, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 10, -11, 7, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 10, -11, 12, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 10, -10, 12, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 1, 10, -10, 13, chunkBounds);
	placeBlockAtCurrentPosition(world, ssId, 2, 10, -11, 13, chunkBounds);

	// 4 Chests
	for (int_t i = 0; i < 4; ++i)
	{
		if (!chestsPlaced[i])
		{
			const int_t offX = Direction::offsetX[i] * 2;
			const int_t offZ = Direction::offsetZ[i] * 2;
			chestsPlaced[i] = generateStructureChestContents(
				world, chunkBounds, random,
				10 + offX, -11, 10 + offZ,
				itemsToGenerateInTemple, templeLootCount,
				2 + random.nextInt(5)
			);
		}
	}

	return true;
}
