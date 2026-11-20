#include "EntityFireworkStarterFX.h"

#include <cmath>

#include "EffectRenderer.h"
#include "EntityFireworkOverlayFX.h"
#include "EntityFireworkSparkFX.h"
#include "EntityLiving.h"
#include "MathHelper.h"
#include "Minecraft.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "World.h"
#include "platform/PlatformConfig.h"

EntityFireworkStarterFX::EntityFireworkStarterFX(World *world, double x, double y, double z,
                                                 double vx, double vy, double vz,
                                                 EffectRenderer *renderer, NBTTagCompound *fireworksTag)
    : EntityFX(world, x, y, z, 0.0, 0.0, 0.0),
      fireworkAge(0),
      effectRenderer(renderer),
      twinkle(false)
{
    motionX = vx;
    motionY = vy;
    motionZ = vz;
    particleMaxAge = 8;

    if (fireworksTag != nullptr && fireworksTag->hasKey("Explosions"))
    {
        NBTTagList *list = fireworksTag->getTagList("Explosions");
        if (list != nullptr && list->tagCount() > 0)
        {
            explosions.reserve(static_cast<size_t>(list->tagCount()));
            for (int_t i = 0; i < list->tagCount(); ++i)
            {
                NBTTagCompound *tag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
                if (tag == nullptr)
                    continue;
                ExplosionDef def;
                def.type = tag->getByte("Type");
                def.trail = tag->getBoolean("Trail");
                def.flicker = tag->getBoolean("Flicker");
                def.colors = tag->getIntArray("Colors");
                if (def.colors.empty())
                    def.colors.push_back(0xFFFFFF);
                def.fadeColors = tag->getIntArray("FadeColors");
                explosions.push_back(std::move(def));
            }
        }
    }

    if (!explosions.empty())
    {
        particleMaxAge = static_cast<int_t>(explosions.size()) * 2 - 1;
        for (const ExplosionDef &def : explosions)
        {
            if (def.flicker)
            {
                twinkle = true;
                particleMaxAge += 15;
                break;
            }
        }
    }
}

void EntityFireworkStarterFX::renderParticle(Tessellator *, float, float, float, float, float, float)
{
}

void EntityFireworkStarterFX::onUpdate()
{
    if (fireworkAge == 0 && !explosions.empty() && worldObj != nullptr)
    {
        const bool farSound = isFarFromCamera();
        bool large = explosions.size() >= 3;
        if (!large)
        {
            for (const ExplosionDef &def : explosions)
            {
                if (def.type == 1)
                {
                    large = true;
                    break;
                }
            }
        }

        const jstring soundName = jstring("fireworks.") + (large ? "largeBlast" : "blast") + (farSound ? "_far" : "");
        worldObj->playSoundEffect(posX, posY, posZ, soundName, 20.0f, 0.95f + rand.nextFloat() * 0.1f);
    }

    if ((fireworkAge % 2) == 0 && !explosions.empty() &&
        static_cast<size_t>(fireworkAge / 2) < explosions.size())
    {
        const ExplosionDef &def = explosions[static_cast<size_t>(fireworkAge / 2)];

        if (def.type == 1)
        {
            createBall(0.5, 4, def.colors, def.fadeColors, def.trail, def.flicker);
        }
        else if (def.type == 2)
        {
            static constexpr double kStarShape[6][2] = {
                { 0.0, 1.0 },
                { 0.3455, 0.309 },
                { 0.9511, 0.309 },
                { 0.3795918367346939, -0.12653061224489795 },
                { 0.6122448979591837, -0.8040816326530612 },
                { 0.0, -0.35918367346938773 }
            };
            createShaped(0.5, kStarShape, 6, def.colors, def.fadeColors, def.trail, def.flicker, false);
        }
        else if (def.type == 3)
        {
            static constexpr double kCreeperShape[12][2] = {
                { 0.0, 0.2 },
                { 0.2, 0.2 },
                { 0.2, 0.6 },
                { 0.6, 0.6 },
                { 0.6, 0.2 },
                { 0.2, 0.2 },
                { 0.2, 0.0 },
                { 0.4, 0.0 },
                { 0.4, -0.6 },
                { 0.2, -0.6 },
                { 0.2, -0.4 },
                { 0.0, -0.4 }
            };
            createShaped(0.5, kCreeperShape, 12, def.colors, def.fadeColors, def.trail, def.flicker, true);
        }
        else if (def.type == 4)
        {
            createBurst(def.colors, def.fadeColors, def.trail, def.flicker);
        }
        else
        {
            createBall(0.25, 2, def.colors, def.fadeColors, def.trail, def.flicker);
        }

        if (effectRenderer != nullptr)
        {
            const int_t primary = def.colors.empty() ? 0xFFFFFF : def.colors[0];
            const float r = static_cast<float>((primary & 0xFF0000) >> 16) / 255.0f;
            const float g = static_cast<float>((primary & 0x00FF00) >> 8) / 255.0f;
            const float b = static_cast<float>(primary & 0x0000FF) / 255.0f;
            EntityFireworkOverlayFX *overlay = new EntityFireworkOverlayFX(worldObj, posX, posY, posZ);
            overlay->setParticleColor(r, g, b);
            effectRenderer->addEffect(overlay);
        }
    }

    ++fireworkAge;
    if (fireworkAge > particleMaxAge)
    {
        if (twinkle && worldObj != nullptr)
        {
            const bool farSound = isFarFromCamera();
            const jstring soundName = jstring("fireworks.") + (farSound ? "twinkle_far" : "twinkle");
            worldObj->playSoundEffect(posX, posY, posZ, soundName, 20.0f, 0.9f + rand.nextFloat() * 0.15f);
        }
        setEntityDead();
    }
}

bool EntityFireworkStarterFX::isFarFromCamera() const
{
    Minecraft *mc = Minecraft::getMinecraft();
    return mc == nullptr || mc->renderViewEntity == nullptr ||
           !(mc->renderViewEntity->getDistanceSq(posX, posY, posZ) < 256.0);
}

void EntityFireworkStarterFX::createParticle(double x, double y, double z,
                                             double vx, double vy, double vz,
                                             const std::vector<int_t> &colors,
                                             const std::vector<int_t> &fadeColors,
                                             bool hasTrail, bool hasTwinkle)
{
    if (effectRenderer == nullptr)
        return;

    EntityFireworkSparkFX *spark = new EntityFireworkSparkFX(worldObj, x, y, z, vx, vy, vz, effectRenderer);
    spark->setTrail(hasTrail);
    spark->setTwinkle(hasTwinkle);
    const int_t color = colors.empty()
        ? 0xFFFFFF
        : colors[static_cast<size_t>(rand.nextInt(static_cast<int_t>(colors.size())))];
    spark->setColour(color);
    if (!fadeColors.empty())
    {
        spark->setFadeColour(fadeColors[static_cast<size_t>(rand.nextInt(static_cast<int_t>(fadeColors.size())))]);
    }
    effectRenderer->addEffect(spark);
}

void EntityFireworkStarterFX::createBall(double speed, int_t size,
                                         const std::vector<int_t> &colors,
                                         const std::vector<int_t> &fadeColors,
                                         bool hasTrail, bool hasTwinkle)
{
    int_t effectiveSize = size;
    int_t skipStride = 1;
#if PLATFORM_XBOX
    const int_t active = effectRenderer != nullptr ? effectRenderer->getLayerParticleCount(0) : 0;
    // Large Ball (size=4) emits 386 sparks in Java; size=3 emits 218 sparks at the exact same radius/speed.
    if (effectiveSize > 3)
        effectiveSize = 3;
    if (active >= 600)
    {
        effectiveSize = std::min<int_t>(effectiveSize, 2);
        skipStride = 4;
    }
    else if (active >= 400)
    {
        effectiveSize = std::min<int_t>(effectiveSize, 2);
        skipStride = 2;
    }
    else if (active >= 300 && effectiveSize > 2)
    {
        effectiveSize = 2;
    }
#endif

    const double baseX = posX;
    const double baseY = posY;
    const double baseZ = posZ;
    int_t emitted = 0;

    for (int_t y = -effectiveSize; y <= effectiveSize; ++y)
    {
        for (int_t x = -effectiveSize; x <= effectiveSize; ++x)
        {
            for (int_t z = -effectiveSize; z <= effectiveSize; ++z)
            {
                if ((emitted++ % skipStride) == 0)
                {
                    const double dx = static_cast<double>(x) + (rand.nextDouble() - rand.nextDouble()) * 0.5;
                    const double dy = static_cast<double>(y) + (rand.nextDouble() - rand.nextDouble()) * 0.5;
                    const double dz = static_cast<double>(z) + (rand.nextDouble() - rand.nextDouble()) * 0.5;
                    const double norm = static_cast<double>(MathHelper::sqrt_double(dx * dx + dy * dy + dz * dz)) / speed + rand.nextGaussian() * 0.05;
                    if (norm > 1.0e-5)
                    {
                        createParticle(baseX, baseY, baseZ, dx / norm, dy / norm, dz / norm,
                                       colors, fadeColors, hasTrail, hasTwinkle);
                    }
                }
                if (y != -effectiveSize && y != effectiveSize && x != -effectiveSize && x != effectiveSize)
                {
                    z += effectiveSize * 2 - 1;
                }
            }
        }
    }
}

void EntityFireworkStarterFX::createShaped(double speed, const double (*shape)[2], int_t pointCount,
                                           const std::vector<int_t> &colors,
                                           const std::vector<int_t> &fadeColors,
                                           bool hasTrail, bool hasTwinkle, bool keepShape)
{
    if (shape == nullptr || pointCount <= 0)
        return;

    int_t planes = 3;
    double interpStep = 0.25;
#if PLATFORM_XBOX
    const int_t active = effectRenderer != nullptr ? effectRenderer->getLayerParticleCount(0) : 0;
    if (active >= 600)
    {
        planes = 1;
        interpStep = 0.5;
    }
    else if (active >= 400)
    {
        planes = 2;
        interpStep = 0.5;
    }
    else if (active >= 300 && pointCount >= 10)
    {
        planes = 2;
        interpStep = 0.3334;
    }
#endif

    const double startX = shape[0][0];
    const double startY = shape[0][1];
    createParticle(posX, posY, posZ, startX * speed, startY * speed, 0.0, colors, fadeColors, hasTrail, hasTwinkle);

    const float baseAngle = rand.nextFloat() * 3.1415927f;
    const double angleMul = keepShape ? 0.034 : 0.34;

    for (int_t plane = 0; plane < planes; ++plane)
    {
        const double angle = static_cast<double>(baseAngle) + static_cast<double>(static_cast<float>(plane) * 3.1415927f) * angleMul;
        const double sinA = std::sin(angle);
        const double cosA = std::cos(angle);
        double prevX = startX;
        double prevY = startY;

        for (int_t p = 1; p < pointCount; ++p)
        {
            const double curX = shape[p][0];
            const double curY = shape[p][1];

            for (double t = interpStep; t <= 1.0001; t += interpStep)
            {
                const double clampedT = t > 1.0 ? 1.0 : t;
                const double rawX = (prevX + (curX - prevX) * clampedT) * speed;
                const double vy = (prevY + (curY - prevY) * clampedT) * speed;
                const double vz = rawX * sinA;
                const double vx = rawX * cosA;

                for (double mirror = -1.0; mirror <= 1.0; mirror += 2.0)
                {
                    createParticle(posX, posY, posZ, vx * mirror, vy, vz * mirror,
                                   colors, fadeColors, hasTrail, hasTwinkle);
                }
            }

            prevX = curX;
            prevY = curY;
        }
    }
}

void EntityFireworkStarterFX::createBurst(const std::vector<int_t> &colors,
                                          const std::vector<int_t> &fadeColors,
                                          bool hasTrail, bool hasTwinkle)
{
    int_t count = 70;
#if PLATFORM_XBOX
    const int_t active = effectRenderer != nullptr ? effectRenderer->getLayerParticleCount(0) : 0;
    if (active >= 550)
        count = 20;
    else if (active >= 350)
        count = 36;
#endif

    const double jitterX = rand.nextGaussian() * 0.05;
    const double jitterZ = rand.nextGaussian() * 0.05;

    for (int_t i = 0; i < count; ++i)
    {
        const double vx = motionX * 0.5 + rand.nextGaussian() * 0.15 + jitterX;
        const double vz = motionZ * 0.5 + rand.nextGaussian() * 0.15 + jitterZ;
        const double vy = motionY * 0.5 + rand.nextDouble() * 0.5;
        createParticle(posX, posY, posZ, vx, vy, vz, colors, fadeColors, hasTrail, hasTwinkle);
    }
}

int_t EntityFireworkStarterFX::getFXLayer()
{
    return 0;
}
