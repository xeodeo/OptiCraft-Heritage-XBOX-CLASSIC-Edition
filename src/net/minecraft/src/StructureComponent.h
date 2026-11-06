#pragma once

#include <memory>
#include <vector>

#include "java/Type.h"

class ChunkPosition;
class Random;
class StructureBoundingBox;
class StructurePieceBlockSelector;
class StructurePieceTreasure;
class WeightedRandomChestContent;
class World;

// net.minecraft.src.StructureComponent
class StructureComponent
{
public:
	explicit StructureComponent(int_t componentType);
	virtual ~StructureComponent();

	virtual void buildComponent(StructureComponent *parent,
	                            std::vector<StructureComponent *> &components,
	                            Random &random);
	virtual bool addComponentParts(World *world, Random &random,
	                               const StructureBoundingBox &chunkBounds) = 0;

	StructureBoundingBox *getBoundingBox() const;
	int_t getComponentType() const;
	static StructureComponent *findIntersecting(const std::vector<StructureComponent *> &components,
	                                            const StructureBoundingBox &bounds);
	virtual ChunkPosition *getCenter() const;

protected:
	bool isLiquidInStructureBoundingBox(World *world, const StructureBoundingBox &chunkBounds) const;
	int_t getXWithOffset(int_t x, int_t z) const;
	int_t getYWithOffset(int_t y) const;
	int_t getZWithOffset(int_t x, int_t z) const;
	int_t getMetadataWithOffset(int_t blockId, int_t metadata) const;

	virtual void placeBlockAtCurrentPosition(World *world, int_t blockId, int_t metadata,
	                                 int_t x, int_t y, int_t z,
	                                 const StructureBoundingBox &chunkBounds) const;
	int_t getBlockIdAtCurrentPosition(World *world, int_t x, int_t y, int_t z,
	                                  const StructureBoundingBox &chunkBounds) const;
	virtual void fillWithBlocks(World *world, const StructureBoundingBox &chunkBounds,
	                    int_t minX, int_t minY, int_t minZ,
	                    int_t maxX, int_t maxY, int_t maxZ,
	                    int_t boundaryBlockId, int_t insideBlockId, bool onlyReplaceNonAir) const;
	void fillWithMetadataBlocks(World *world, const StructureBoundingBox &chunkBounds, int_t minX, int_t minY, int_t minZ, int_t maxX, int_t maxY, int_t maxZ, int_t boundaryBlockId, int_t boundaryMetadata, int_t insideBlockId, int_t insideMetadata, bool onlyReplaceNonAir) const;
	void fillWithAir(World *world, const StructureBoundingBox &chunkBounds, int_t minX, int_t minY, int_t minZ, int_t maxX, int_t maxY, int_t maxZ) const;
	bool generateStructureChestContents(World *world, const StructureBoundingBox &chunkBounds, Random &random, int_t x, int_t y, int_t z, const WeightedRandomChestContent *loot, std::size_t lootCount, int_t rolls) const;
	bool generateStructureDispenserContents(World *world, const StructureBoundingBox &chunkBounds, Random &random, int_t x, int_t y, int_t z, int_t facing, const WeightedRandomChestContent *loot, std::size_t lootCount, int_t rolls) const;
	void fillWithRandomizedBlocks(World *world, const StructureBoundingBox &chunkBounds,
	                              int_t minX, int_t minY, int_t minZ,
	                              int_t maxX, int_t maxY, int_t maxZ,
	                              bool onlyReplaceNonAir, Random &random,
	                              StructurePieceBlockSelector &selector) const;
	void randomlyFillWithBlocks(World *world, const StructureBoundingBox &chunkBounds,
	                            Random &random, float chance,
	                            int_t minX, int_t minY, int_t minZ,
	                            int_t maxX, int_t maxY, int_t maxZ,
	                            int_t boundaryBlockId, int_t insideBlockId,
	                            bool onlyReplaceNonAir) const;
	void randomlyPlaceBlock(World *world, const StructureBoundingBox &chunkBounds,
	                        Random &random, float chance,
	                        int_t x, int_t y, int_t z,
	                        int_t blockId, int_t metadata) const;
	void randomlyRareFillWithBlocks(World *world, const StructureBoundingBox &chunkBounds,
	                                int_t minX, int_t minY, int_t minZ,
	                                int_t maxX, int_t maxY, int_t maxZ,
	                                int_t blockId, bool onlyReplaceNonAir) const;
	void clearCurrentPositionBlocksUpwards(World *world, int_t x, int_t y, int_t z,
	                                       const StructureBoundingBox &chunkBounds) const;
	virtual void fillCurrentPositionBlocksDownwards(World *world, int_t blockId, int_t metadata,
	                                        int_t x, int_t y, int_t z,
	                                        const StructureBoundingBox &chunkBounds) const;
	void createTreasureChestAtCurrentPosition(World *world, const StructureBoundingBox &chunkBounds,
	                                          Random &random, int_t x, int_t y, int_t z,
	                                          const std::vector<StructurePieceTreasure> &loot,
	                                          int_t rolls) const;
	void placeDoorAtCurrentPosition(World *world, const StructureBoundingBox &chunkBounds,
	                                Random &random, int_t x, int_t y, int_t z,
	                                int_t direction) const;

	std::unique_ptr<StructureBoundingBox> boundingBox;
	int_t coordBaseMode;
	int_t componentType;

private:
	static void fillTreasureChestWithLoot(Random &random,
	                                     const std::vector<StructurePieceTreasure> &loot,
	                                     class TileEntityChest *chest,
	                                     int_t rolls);
};
