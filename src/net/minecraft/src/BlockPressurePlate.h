#pragma once
#include "BlockBasePressurePlate.h"
#include "EnumMobType.h"

// net.minecraft.src.BlockPressurePlate
class BlockPressurePlate : public BlockBasePressurePlate
{
public:
	BlockPressurePlate(int_t id, int_t textureIndex, EnumMobType mobType, Material *material);

protected:
	int_t getPlateState(World *world, int_t x, int_t y, int_t z) override;
	int_t getPowerSupply(int_t meta) override;
	int_t getMetaFromWeight(int_t weight) override;

private:
	EnumMobType triggerMobType;
};
