#include "EntityFireworkOverlayFX.h"

#include "MathHelper.h"
#include "Tessellator.h"

EntityFireworkOverlayFX::EntityFireworkOverlayFX(World *world, double x, double y, double z)
    : EntityFX(world, x, y, z)
{
    particleMaxAge = 4;
}

void EntityFireworkOverlayFX::renderParticle(Tessellator *tessellator, float partialTick,
                                             float rotationX, float rotationXZ, float rotationZ,
                                             float rotationYZ, float rotationXY)
{
    const float minU = 0.25f;
    const float maxU = minU + 0.25f;
    const float minV = 0.125f;
    const float maxV = minV + 0.25f;
    const float phase = (static_cast<float>(particleAge) + partialTick - 1.0f) * 0.25f;
    const float scale = 7.1f * MathHelper::sin(phase * 3.1415927f);
    if (scale <= 0.0f)
        return;

    particleAlpha = 0.6f - phase * 0.5f;
    if (particleAlpha <= 0.0f)
        return;

    const float rx = interpolateRenderCoordinate(prevPosX, posX, interpPosX, partialTick);
    const float ry = interpolateRenderCoordinate(prevPosY, posY, interpPosY, partialTick);
    const float rz = interpolateRenderCoordinate(prevPosZ, posZ, interpPosZ, partialTick);

    tessellator->setColorRGBA_F(particleRed, particleGreen, particleBlue, particleAlpha);
    tessellator->addVertexWithUV(rx - rotationX * scale - rotationYZ * scale,
                                 ry - rotationXZ * scale,
                                 rz - rotationZ * scale - rotationXY * scale, maxU, maxV);
    tessellator->addVertexWithUV(rx - rotationX * scale + rotationYZ * scale,
                                 ry + rotationXZ * scale,
                                 rz - rotationZ * scale + rotationXY * scale, maxU, minV);
    tessellator->addVertexWithUV(rx + rotationX * scale + rotationYZ * scale,
                                 ry + rotationXZ * scale,
                                 rz + rotationZ * scale + rotationXY * scale, minU, minV);
    tessellator->addVertexWithUV(rx + rotationX * scale - rotationYZ * scale,
                                 ry - rotationXZ * scale,
                                 rz + rotationZ * scale - rotationXY * scale, minU, maxV);
}

int_t EntityFireworkOverlayFX::getBrightnessForRender(float)
{
    return 15728880;
}

float EntityFireworkOverlayFX::getEntityBrightness(float)
{
    return 1.0f;
}
