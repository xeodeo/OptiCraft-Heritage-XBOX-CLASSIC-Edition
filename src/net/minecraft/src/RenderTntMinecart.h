#pragma once

#include "RenderMinecart.h"

class Block;
class EntityMinecart;

class RenderTntMinecart : public RenderMinecart
{
public:
    RenderTntMinecart();
    ~RenderTntMinecart() override = default;

protected:
    void renderBlockInMinecart(EntityMinecart* entityMinecart, float partialTicks, Block* block, int_t metadata) override;
};
