#pragma once

#include "EntityFX.h"

class EffectRenderer;
class Tessellator;
class World;

// net.minecraft.src.EntityFireworkSparkFX
class EntityFireworkSparkFX : public EntityFX
{
public:
    EntityFireworkSparkFX(World *world, double x, double y, double z,
                          double vx, double vy, double vz, EffectRenderer *effectRenderer);

    void setTrail(bool enable);
    void setTwinkle(bool enable);
    void setColour(int_t rgb);
    void setFadeColour(int_t rgb);

    AxisAlignedBB *getBoundingBox() override;
    bool canBePushed() override;
    void renderParticle(Tessellator *tessellator, float partialTick,
                        float rotationX, float rotationXZ, float rotationZ,
                        float rotationYZ, float rotationXY) override;
    void onUpdate() override;
    int_t getBrightnessForRender(float partialTick) override;
    float getEntityBrightness(float partialTick) override;

private:
    int_t baseTextureIndex;
    bool trail;
    bool twinkle;
    EffectRenderer *effectRenderer;
    float fadeColourRed;
    float fadeColourGreen;
    float fadeColourBlue;
    bool hasFadeColour;
};
