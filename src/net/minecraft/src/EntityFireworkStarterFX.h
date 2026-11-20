#pragma once

#include <vector>

#include "EntityFX.h"

class EffectRenderer;
class NBTTagCompound;
class Tessellator;
class World;

// net.minecraft.src.EntityFireworkStarterFX
class EntityFireworkStarterFX : public EntityFX
{
public:
    EntityFireworkStarterFX(World *world, double x, double y, double z,
                            double vx, double vy, double vz,
                            EffectRenderer *effectRenderer, NBTTagCompound *fireworksTag);

    void renderParticle(Tessellator *tessellator, float partialTick,
                        float rotationX, float rotationXZ, float rotationZ,
                        float rotationYZ, float rotationXY) override;
    void onUpdate() override;
    int_t getFXLayer() override;

private:
    struct ExplosionDef
    {
        byte_t type = 0;
        bool trail = false;
        bool flicker = false;
        std::vector<int_t> colors;
        std::vector<int_t> fadeColors;
    };

    bool isFarFromCamera() const;
    void createParticle(double x, double y, double z, double vx, double vy, double vz,
                        const std::vector<int_t> &colors, const std::vector<int_t> &fadeColors,
                        bool trail, bool twinkle);
    void createBall(double speed, int_t size, const std::vector<int_t> &colors,
                    const std::vector<int_t> &fadeColors, bool trail, bool twinkle);
    void createShaped(double speed, const double (*shape)[2], int_t pointCount,
                      const std::vector<int_t> &colors, const std::vector<int_t> &fadeColors,
                      bool trail, bool twinkle, bool keepShape);
    void createBurst(const std::vector<int_t> &colors, const std::vector<int_t> &fadeColors,
                     bool trail, bool twinkle);

    int_t fireworkAge;
    EffectRenderer *effectRenderer;
    std::vector<ExplosionDef> explosions;
    bool twinkle;
};
