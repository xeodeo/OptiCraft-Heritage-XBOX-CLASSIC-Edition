#pragma once

#include "EntityFX.h"

class Tessellator;
class World;

// net.minecraft.src.EntityFireworkOverlayFX
class EntityFireworkOverlayFX : public EntityFX
{
public:
    EntityFireworkOverlayFX(World *world, double x, double y, double z);

    void renderParticle(Tessellator *tessellator, float partialTick,
                        float rotationX, float rotationXZ, float rotationZ,
                        float rotationYZ, float rotationXY) override;
    int_t getBrightnessForRender(float partialTick) override;
    float getEntityBrightness(float partialTick) override;
};
