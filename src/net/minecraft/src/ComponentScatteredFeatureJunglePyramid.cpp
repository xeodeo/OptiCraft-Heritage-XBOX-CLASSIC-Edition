#include "ComponentScatteredFeatureJunglePyramid.h"

#include "Block.h"
#include "BlockLever.h"
#include "BlockVine.h"
#include "Item.h"
#include "StructureBoundingBox.h"
#include "WeightedRandomChestContent.h"
#include "World.h"
#include "java/Random.h"

StructureScatteredFeatureStones ComponentScatteredFeatureJunglePyramid::junglePyramidsRandomScatteredStones;

ComponentScatteredFeatureJunglePyramid::ComponentScatteredFeatureJunglePyramid(Random &random, int_t x, int_t z)
	: ComponentScatteredFeature(random, x, 64, z, 12, 10, 15),
	  placedMainChest(false),
	  placedHiddenChest(false),
	  placedTrap1(false),
	  placedTrap2(false)
{
}

bool ComponentScatteredFeatureJunglePyramid::addComponentParts(World *world, Random &random, const StructureBoundingBox &chunkBounds)
{
	if (!func_74935_a(world, chunkBounds, 0))
		return false;

	static const WeightedRandomChestContent junglePyramidsChestContents[] = {
		WeightedRandomChestContent(Item::diamond != nullptr ? Item::diamond->itemID : 264, 0, 1, 3, 3),
		WeightedRandomChestContent(Item::ingotIron != nullptr ? Item::ingotIron->itemID : 265, 0, 1, 5, 10),
		WeightedRandomChestContent(Item::ingotGold != nullptr ? Item::ingotGold->itemID : 266, 0, 2, 7, 15),
		WeightedRandomChestContent(Item::emerald != nullptr ? Item::emerald->itemID : 388, 0, 1, 3, 2),
		WeightedRandomChestContent(Item::bone != nullptr ? Item::bone->itemID : 352, 0, 4, 6, 20),
		WeightedRandomChestContent(Item::rottenFlesh != nullptr ? Item::rottenFlesh->itemID : 367, 0, 3, 7, 16)
	};
	constexpr std::size_t chestLootCount = sizeof(junglePyramidsChestContents) / sizeof(junglePyramidsChestContents[0]);

	static const WeightedRandomChestContent junglePyramidsDispenserContents[] = {
		WeightedRandomChestContent(Item::arrow != nullptr ? Item::arrow->itemID : 262, 0, 2, 7, 30)
	};
	constexpr std::size_t dispenserLootCount = sizeof(junglePyramidsDispenserContents) / sizeof(junglePyramidsDispenserContents[0]);

	const int_t stairsCobbleId = Block::stairsCobblestone != nullptr ? Block::stairsCobblestone->blockID : 67;
	const int_t tripwireSourceId = Block::tripWireSource != nullptr ? Block::tripWireSource->blockID : 131;
	const int_t tripwireId = Block::tripWire != nullptr ? Block::tripWire->blockID : 132;
	const int_t redstoneWireId = Block::redstoneWire != nullptr ? Block::redstoneWire->blockID : 55;
	const int_t mossyCobbleId = Block::cobblestoneMossy != nullptr ? Block::cobblestoneMossy->blockID : 48;
	const int_t vineId = Block::vine != nullptr ? Block::vine->blockID : 106;
	const int_t stoneBrickId = Block::stoneBrick != nullptr ? Block::stoneBrick->blockID : 98;
	const int_t leverId = Block::lever != nullptr ? Block::lever->blockID : 69;
	const int_t stickyPistonId = Block::pistonStickyBase != nullptr ? Block::pistonStickyBase->blockID : 29;
	const int_t repeaterId = Block::redstoneRepeaterIdle != nullptr ? Block::redstoneRepeaterIdle->blockID : 93;

	const int_t stairsNorth = getMetadataWithOffset(stairsCobbleId, 3);
	const int_t stairsSouth = getMetadataWithOffset(stairsCobbleId, 2);
	const int_t stairsEast = getMetadataWithOffset(stairsCobbleId, 0);
	const int_t stairsWest = getMetadataWithOffset(stairsCobbleId, 1);

	fillWithRandomizedBlocks(world, chunkBounds, 0, -4, 0, scatteredFeatureSizeX - 1, 0, scatteredFeatureSizeZ - 1, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 2, 1, 2, 9, 2, 2, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 2, 1, 12, 9, 2, 12, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 2, 1, 3, 2, 2, 11, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 9, 1, 3, 9, 2, 11, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 1, 3, 1, 10, 6, 1, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 1, 3, 13, 10, 6, 13, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 1, 3, 2, 1, 6, 12, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 10, 3, 2, 10, 6, 12, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 2, 3, 2, 9, 3, 12, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 2, 6, 2, 9, 6, 12, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 3, 7, 3, 8, 7, 11, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 4, 8, 4, 7, 8, 10, false, random, junglePyramidsRandomScatteredStones);

	fillWithAir(world, chunkBounds, 3, 1, 3, 8, 2, 11);
	fillWithAir(world, chunkBounds, 4, 3, 6, 7, 3, 9);
	fillWithAir(world, chunkBounds, 2, 4, 2, 9, 5, 12);
	fillWithAir(world, chunkBounds, 4, 6, 5, 7, 6, 9);
	fillWithAir(world, chunkBounds, 5, 7, 6, 6, 7, 8);
	fillWithAir(world, chunkBounds, 5, 1, 2, 6, 2, 2);
	fillWithAir(world, chunkBounds, 5, 2, 12, 6, 2, 12);
	fillWithAir(world, chunkBounds, 5, 5, 1, 6, 5, 1);
	fillWithAir(world, chunkBounds, 5, 5, 13, 6, 5, 13);
	placeBlockAtCurrentPosition(world, 0, 0, 1, 5, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 10, 5, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 1, 5, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, 0, 0, 10, 5, 9, chunkBounds);

	for (int_t z = 0; z <= 14; z += 14)
	{
		fillWithRandomizedBlocks(world, chunkBounds, 2, 4, z, 2, 5, z, false, random, junglePyramidsRandomScatteredStones);
		fillWithRandomizedBlocks(world, chunkBounds, 4, 4, z, 4, 5, z, false, random, junglePyramidsRandomScatteredStones);
		fillWithRandomizedBlocks(world, chunkBounds, 7, 4, z, 7, 5, z, false, random, junglePyramidsRandomScatteredStones);
		fillWithRandomizedBlocks(world, chunkBounds, 9, 4, z, 9, 5, z, false, random, junglePyramidsRandomScatteredStones);
	}

	fillWithRandomizedBlocks(world, chunkBounds, 5, 6, 0, 6, 6, 0, false, random, junglePyramidsRandomScatteredStones);

	for (int_t x = 0; x <= 11; x += 11)
	{
		for (int_t z = 2; z <= 12; z += 2)
		{
			fillWithRandomizedBlocks(world, chunkBounds, x, 4, z, x, 5, z, false, random, junglePyramidsRandomScatteredStones);
		}
		fillWithRandomizedBlocks(world, chunkBounds, x, 6, 5, x, 6, 5, false, random, junglePyramidsRandomScatteredStones);
		fillWithRandomizedBlocks(world, chunkBounds, x, 6, 9, x, 6, 9, false, random, junglePyramidsRandomScatteredStones);
	}

	fillWithRandomizedBlocks(world, chunkBounds, 2, 7, 2, 2, 9, 2, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 9, 7, 2, 9, 9, 2, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 2, 7, 12, 2, 9, 12, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 9, 7, 12, 9, 9, 12, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 4, 9, 4, 4, 9, 4, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 7, 9, 4, 7, 9, 4, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 4, 9, 10, 4, 9, 10, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 7, 9, 10, 7, 9, 10, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 5, 9, 7, 6, 9, 7, false, random, junglePyramidsRandomScatteredStones);

	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 5, 9, 6, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 6, 9, 6, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsSouth, 5, 9, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsSouth, 6, 9, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 4, 0, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 5, 0, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 6, 0, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 7, 0, 0, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 4, 1, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 4, 2, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 4, 3, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 7, 1, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 7, 2, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsNorth, 7, 3, 10, chunkBounds);

	fillWithRandomizedBlocks(world, chunkBounds, 4, 1, 9, 4, 1, 9, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 7, 1, 9, 7, 1, 9, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 4, 1, 10, 7, 2, 10, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 5, 4, 5, 6, 4, 5, false, random, junglePyramidsRandomScatteredStones);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsEast, 4, 4, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, stairsCobbleId, stairsWest, 7, 4, 5, chunkBounds);

	for (int_t step = 0; step < 4; ++step)
	{
		placeBlockAtCurrentPosition(world, stairsCobbleId, stairsSouth, 5, 0 - step, 6 + step, chunkBounds);
		placeBlockAtCurrentPosition(world, stairsCobbleId, stairsSouth, 6, 0 - step, 6 + step, chunkBounds);
		fillWithAir(world, chunkBounds, 5, 0 - step, 7 + step, 6, 0 - step, 9 + step);
	}

	fillWithAir(world, chunkBounds, 1, -3, 12, 10, -1, 13);
	fillWithAir(world, chunkBounds, 1, -3, 1, 3, -1, 13);
	fillWithAir(world, chunkBounds, 1, -3, 1, 9, -1, 5);

	for (int_t z = 1; z <= 13; z += 2)
	{
		fillWithRandomizedBlocks(world, chunkBounds, 1, -3, z, 1, -2, z, false, random, junglePyramidsRandomScatteredStones);
	}

	for (int_t z = 2; z <= 12; z += 2)
	{
		fillWithRandomizedBlocks(world, chunkBounds, 1, -1, z, 3, -1, z, false, random, junglePyramidsRandomScatteredStones);
	}

	fillWithRandomizedBlocks(world, chunkBounds, 2, -2, 1, 5, -2, 1, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 7, -2, 1, 9, -2, 1, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 6, -3, 1, 6, -3, 1, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 6, -1, 1, 6, -1, 1, false, random, junglePyramidsRandomScatteredStones);

	// Tripwire trap 1
	placeBlockAtCurrentPosition(world, tripwireSourceId, getMetadataWithOffset(tripwireSourceId, 3) | 4, 1, -3, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, tripwireSourceId, getMetadataWithOffset(tripwireSourceId, 1) | 4, 4, -3, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, tripwireId, 4, 2, -3, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, tripwireId, 4, 3, -3, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 5, -3, 7, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 5, -3, 6, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 5, -3, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 5, -3, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 5, -3, 3, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 5, -3, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 5, -3, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 4, -3, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 3, -3, 1, chunkBounds);

	if (!placedTrap1)
	{
		placedTrap1 = generateStructureDispenserContents(world, chunkBounds, random, 3, -2, 1, 2, junglePyramidsDispenserContents, dispenserLootCount, 2);
	}

	placeBlockAtCurrentPosition(world, vineId, 15, 3, -2, 2, chunkBounds);

	// Tripwire trap 2
	placeBlockAtCurrentPosition(world, tripwireSourceId, getMetadataWithOffset(tripwireSourceId, 2) | 4, 7, -3, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, tripwireSourceId, getMetadataWithOffset(tripwireSourceId, 0) | 4, 7, -3, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, tripwireId, 4, 7, -3, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, tripwireId, 4, 7, -3, 3, chunkBounds);
	placeBlockAtCurrentPosition(world, tripwireId, 4, 7, -3, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 8, -3, 6, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 9, -3, 6, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 9, -3, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 9, -3, 4, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 9, -2, 4, chunkBounds);

	if (!placedTrap2)
	{
		placedTrap2 = generateStructureDispenserContents(world, chunkBounds, random, 9, -2, 3, 4, junglePyramidsDispenserContents, dispenserLootCount, 2);
	}

	placeBlockAtCurrentPosition(world, vineId, 15, 8, -1, 3, chunkBounds);
	placeBlockAtCurrentPosition(world, vineId, 15, 8, -2, 3, chunkBounds);

	// Chest 1
	if (!placedMainChest)
	{
		placedMainChest = generateStructureChestContents(world, chunkBounds, random, 8, -3, 3,
			junglePyramidsChestContents, chestLootCount, 2 + random.nextInt(5));
	}

	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 9, -3, 2, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 8, -3, 1, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 4, -3, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 5, -2, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 5, -1, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 6, -3, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 7, -2, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 7, -1, 5, chunkBounds);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 8, -3, 5, chunkBounds);
	fillWithRandomizedBlocks(world, chunkBounds, 9, -1, 1, 9, -1, 5, false, random, junglePyramidsRandomScatteredStones);
	fillWithAir(world, chunkBounds, 8, -3, 8, 10, -1, 10);

	// Lever puzzle
	placeBlockAtCurrentPosition(world, stoneBrickId, 3, 8, -2, 11, chunkBounds);
	placeBlockAtCurrentPosition(world, stoneBrickId, 3, 9, -2, 11, chunkBounds);
	placeBlockAtCurrentPosition(world, stoneBrickId, 3, 10, -2, 11, chunkBounds);

	const int_t leverMeta = BlockLever::invertMetadata(getMetadataWithOffset(leverId, 2));
	placeBlockAtCurrentPosition(world, leverId, leverMeta, 8, -2, 12, chunkBounds);
	placeBlockAtCurrentPosition(world, leverId, leverMeta, 9, -2, 12, chunkBounds);
	placeBlockAtCurrentPosition(world, leverId, leverMeta, 10, -2, 12, chunkBounds);

	fillWithRandomizedBlocks(world, chunkBounds, 8, -3, 8, 8, -3, 10, false, random, junglePyramidsRandomScatteredStones);
	fillWithRandomizedBlocks(world, chunkBounds, 10, -3, 8, 10, -3, 10, false, random, junglePyramidsRandomScatteredStones);
	placeBlockAtCurrentPosition(world, mossyCobbleId, 0, 10, -2, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 8, -2, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 8, -2, 10, chunkBounds);
	placeBlockAtCurrentPosition(world, redstoneWireId, 0, 10, -1, 9, chunkBounds);
	placeBlockAtCurrentPosition(world, stickyPistonId, 1, 9, -2, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, stickyPistonId, getMetadataWithOffset(stickyPistonId, 4), 10, -2, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, stickyPistonId, getMetadataWithOffset(stickyPistonId, 4), 10, -1, 8, chunkBounds);
	placeBlockAtCurrentPosition(world, repeaterId, getMetadataWithOffset(repeaterId, 2), 10, -2, 10, chunkBounds);

	// Hidden chest
	if (!placedHiddenChest)
	{
		placedHiddenChest = generateStructureChestContents(world, chunkBounds, random, 9, -3, 10,
			junglePyramidsChestContents, chestLootCount, 2 + random.nextInt(5));
	}

	return true;
}
