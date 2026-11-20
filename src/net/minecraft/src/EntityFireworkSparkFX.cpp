#include "EntityFireworkSparkFX.h"

#include "EffectRenderer.h"
#include "MathHelper.h"
#include "World.h"
#include "platform/PlatformConfig.h"

EntityFireworkSparkFX::EntityFireworkSparkFX(World *world, double x, double y, double z,
                                             double vx, double vy, double vz,
                                             EffectRenderer *renderer)
    : EntityFX(world, x, y, z),
      baseTextureIndex(160),
      trail(false),
      twinkle(false),
      effectRenderer(renderer),
      fadeColourRed(0.0f),
      fadeColourGreen(0.0f),
      fadeColourBlue(0.0f),
      hasFadeColour(false)
{
    motionX = vx;
    motionY = vy;
    motionZ = vz;
    particleScale *= 0.75f;
    particleMaxAge = 48 + rand.nextInt(12);
    noClip = false;
    setParticleTextureIndex(baseTextureIndex + 7);
}

void EntityFireworkSparkFX::setTrail(bool enable)
{
    trail = enable;
}

void EntityFireworkSparkFX::setTwinkle(bool enable)
{
    twinkle = enable;
}

void EntityFireworkSparkFX::setColour(int_t rgb)
{
    const float r = static_cast<float>((rgb & 0xFF0000) >> 16) / 255.0f;
    const float g = static_cast<float>((rgb & 0x00FF00) >> 8) / 255.0f;
    const float b = static_cast<float>(rgb & 0x0000FF) / 255.0f;
    setParticleColor(r, g, b);
}

void EntityFireworkSparkFX::setFadeColour(int_t rgb)
{
    fadeColourRed = static_cast<float>((rgb & 0xFF0000) >> 16) / 255.0f;
    fadeColourGreen = static_cast<float>((rgb & 0x00FF00) >> 8) / 255.0f;
    fadeColourBlue = static_cast<float>(rgb & 0x0000FF) / 255.0f;
    hasFadeColour = true;
}

AxisAlignedBB *EntityFireworkSparkFX::getBoundingBox()
{
    return nullptr;
}

bool EntityFireworkSparkFX::canBePushed()
{
    return false;
}

void EntityFireworkSparkFX::renderParticle(Tessellator *tessellator, float partialTick,
                                           float rotationX, float rotationXZ, float rotationZ,
                                           float rotationYZ, float rotationXY)
{
    if (!twinkle || particleAge < particleMaxAge / 3 || ((particleAge + particleMaxAge) / 3) % 2 == 0)
    {
        EntityFX::renderParticle(tessellator, partialTick, rotationX, rotationXZ, rotationZ, rotationYZ, rotationXY);
    }
}

void EntityFireworkSparkFX::onUpdate()
{
    prevPosX = posX;
    prevPosY = posY;
    prevPosZ = posZ;
    if (particleAge++ >= particleMaxAge)
    {
        setEntityDead();
        return;
    }

    if (particleAge > particleMaxAge / 2)
    {
        setAlphaF(1.0f - (static_cast<float>(particleAge) - static_cast<float>(particleMaxAge / 2)) / static_cast<float>(particleMaxAge));
        if (hasFadeColour)
        {
            particleRed += (fadeColourRed - particleRed) * 0.2f;
            particleGreen += (fadeColourGreen - particleGreen) * 0.2f;
            particleBlue += (fadeColourBlue - particleBlue) * 0.2f;
        }
    }

    int_t frame = 7 - (particleAge * 8 / particleMaxAge);
    if (frame < 0) frame = 0;
    if (frame > 7) frame = 7;
    setParticleTextureIndex(baseTextureIndex + frame);

    motionY -= 0.004;
    const double nextX = posX + motionX;
    const double nextY = posY + motionY;
    const double nextZ = posZ + motionZ;
    if (worldObj != nullptr &&
        worldObj->getBlockId(MathHelper::floor_double(nextX),
                             MathHelper::floor_double(nextY),
                             MathHelper::floor_double(nextZ)) == 0)
    {
        boundingBox->offset(motionX, motionY, motionZ);
        posX = (boundingBox->minX + boundingBox->maxX) * 0.5;
        posY = boundingBox->minY + static_cast<double>(yOffset) - static_cast<double>(ySize);
        posZ = (boundingBox->minZ + boundingBox->maxZ) * 0.5;
        onGround = false;
    }
    else
    {
        moveEntity(motionX, motionY, motionZ);
    }
    motionX *= 0.9100000262260437;
    motionY *= 0.9100000262260437;
    motionZ *= 0.9100000262260437;
    if (onGround)
    {
        motionX *= 0.699999988079071;
        motionZ *= 0.699999988079071;
    }

    if (trail && effectRenderer != nullptr && particleAge < particleMaxAge / 2)
    {
        int_t cadence = 2;
#if PLATFORM_XBOX
        const int_t active = effectRenderer->getLayerParticleCount(0);
        if (active >= 480)
            cadence = 0;
        else if (active >= 300)
            cadence = 4;
#endif
        if (cadence > 0 && ((particleAge + particleMaxAge) % cadence) == 0)
        {
            EntityFireworkSparkFX *child = new EntityFireworkSparkFX(worldObj, posX, posY, posZ, 0.0, 0.0, 0.0, effectRenderer);
            child->setParticleColor(particleRed, particleGreen, particleBlue);
            child->particleAge = child->particleMaxAge / 2;
            if (hasFadeColour)
            {
                child->hasFadeColour = true;
                child->fadeColourRed = fadeColourRed;
                child->fadeColourGreen = fadeColourGreen;
                child->fadeColourBlue = fadeColourBlue;
            }
            child->twinkle = twinkle;
            effectRenderer->addEffect(child);
        }
    }
}

int_t EntityFireworkSparkFX::getBrightnessForRender(float)
{
    return 15728880;
}

float EntityFireworkSparkFX::getEntityBrightness(float)
{
    return 1.0f;
}
