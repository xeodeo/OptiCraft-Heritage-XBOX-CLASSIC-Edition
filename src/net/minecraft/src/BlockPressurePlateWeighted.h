#pragma once
#include "BlockBasePressurePlate.h"

// net.minecraft.src.BlockPressurePlateWeighted
class BlockPressurePlateWeighted : public BlockBasePressurePlate
{
public:
	BlockPressurePlateWeighted(int_t id, int_t textureIndex, Material *material, int_t maxItems);

	int_t tickRate() override;

protected:
	int_t getPlateState(World *world, int_t x, int_t y, int_t z) override;
	int_t getPowerSupply(int_t meta) override;
	int_t getMetaFromWeight(int_t weight) override;

private:
	const int_t maxItemsWeighted;
};
