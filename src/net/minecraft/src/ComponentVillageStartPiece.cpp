#include "ComponentVillageStartPiece.h"

#include "BiomeGenBase.h"
#include "StructureVillagePieceWeight.h"
#include "WorldChunkManager.h"
#include "java/Random.h"

ComponentVillageStartPiece::ComponentVillageStartPiece(
	WorldChunkManager *manager, int_t, Random &random, int_t x, int_t z,
	const std::vector<StructureVillagePieceWeight *> &weights, int_t terrainTypeValue)
	: ComponentVillageWell(0, random, x, z),
	  worldChunkMngr(manager), inDesert(false), terrainType(terrainTypeValue), structVillagePieceWeight(nullptr),
	  structureVillageWeightedPieceList(weights)
{
	BiomeGenBase *biome = manager != nullptr ? manager->getBiomeGenAt(x, z) : nullptr;
	inDesert = biome == BiomeGenBase::desert || biome == BiomeGenBase::desertHills;
	startPiece = this;
	ownedVillagePieceWeights.reserve(weights.size());
	for (StructureVillagePieceWeight *weight : weights)
		ownedVillagePieceWeights.emplace_back(weight);
}

ComponentVillageStartPiece::~ComponentVillageStartPiece() = default;

WorldChunkManager *ComponentVillageStartPiece::getWorldChunkManager() const
{
	return worldChunkMngr;
}
