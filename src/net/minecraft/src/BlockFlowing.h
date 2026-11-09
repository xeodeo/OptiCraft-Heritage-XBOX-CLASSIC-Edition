#pragma once
#include "BlockFluid.h"

// net.minecraft.src.BlockFlowing
class BlockFlowing : public BlockFluid
{
public:
    BlockFlowing(int_t i, Material *material);
    void updateTick(World *world, int_t i, int_t j, int_t k, Random &random) override;
    void onBlockAdded(World *world, int_t i, int_t j, int_t k) override;
    bool func_82506_l() override;

protected:
    int_t getSmallestFlowDecay(World *world, int_t i, int_t j, int_t k, int_t l);

    int_t numAdjacentSources;
    bool  isOptimalFlowDirection[4];
    int_t flowCost[4];

private:
    void getFlowDirection(World *world, int_t i, int_t j, int_t k);
    void flowIntoBlock(World *world, int_t i, int_t j, int_t k, int_t l);
    int_t calculateFlowCost(World *world, int_t i, int_t j, int_t k, int_t l, int_t i1);
    bool *getOptimalFlowDirections(World *world, int_t i, int_t j, int_t k);
    bool blockBlocksFlow(World *world, int_t i, int_t j, int_t k);
    bool blockIdBlocksFlow(int_t blockId) const;
    // True when the neighbour cannot be flowed into: it blocks flow, or it is
    // already a source of this liquid. One block read instead of the two or
    // three the same test used to make through World.
    bool neighborRejectsFlow(World *world, int_t i, int_t j, int_t k);
    bool liquidCanDisplaceBlock(World *world, int_t i, int_t j, int_t k);
};
