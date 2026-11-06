#pragma once

#include <vector>

#include "StructureComponent.h"

class ComponentVillageStartPiece;
class Random;
class StructureBoundingBox;
class World;

// net.minecraft.src.ComponentVillage
class ComponentVillage : public StructureComponent
{
public:
	explicit ComponentVillage(int_t componentType);

protected:
	StructureComponent *getNextComponentNN(ComponentVillageStartPiece *start,
	                                       std::vector<StructureComponent *> &components,
	                                       Random &random, int_t yOffset, int_t sideOffset);
	StructureComponent *getNextComponentPP(ComponentVillageStartPiece *start,
	                                       std::vector<StructureComponent *> &components,
	                                       Random &random, int_t yOffset, int_t sideOffset);
	int_t getAverageGroundLevel(World *world, const StructureBoundingBox &chunkBounds) const;
	static bool canVillageGoDeeper(const StructureBoundingBox *bounds);
	void spawnVillagers(World *world, const StructureBoundingBox &chunkBounds,
	                    int_t x, int_t y, int_t z, int_t count);
	virtual int_t getVillagerType(int_t index) const;
	int_t getBiomeSpecificBlock(int_t blockId, int_t metadata) const;
	int_t getBiomeSpecificBlockMetadata(int_t blockId, int_t metadata) const;
	void placeBlockAtCurrentPosition(World *world, int_t blockId, int_t metadata,
	                                 int_t x, int_t y, int_t z,
	                                 const StructureBoundingBox &chunkBounds) const override;
	void fillWithBlocks(World *world, const StructureBoundingBox &chunkBounds,
	                    int_t minX, int_t minY, int_t minZ,
	                    int_t maxX, int_t maxY, int_t maxZ,
	                    int_t boundaryBlockId, int_t insideBlockId, bool onlyReplaceNonAir) const override;
	void fillCurrentPositionBlocksDownwards(World *world, int_t blockId, int_t metadata,
	                                        int_t x, int_t y, int_t z,
	                                        const StructureBoundingBox &chunkBounds) const override;

public:
	// Java passes it to every piece's constructor; here StructureVillagePieces sets it.
	ComponentVillageStartPiece *startPiece = nullptr;

private:
	int_t villagersSpawned;
};
