#pragma once

#include "EntityAIBase.h"
#include "java/Type.h"

class EntityLiving;

// net.minecraft.src.EntityAIControlledByPlayer
class EntityAIControlledByPlayer : public EntityAIBase
{
public:
    EntityAIControlledByPlayer(EntityLiving *entity, float speed);

    void startExecuting() override;
    void resetTask() override;
    bool shouldExecute() override;
    void updateTask() override;

    bool isSpeedBoosted() const;
    void boostSpeed();
    bool isControlledByPlayer() const;

private:
    bool func_98216_b(int_t blockId);

    EntityLiving *thisEntity = nullptr;
    float maxSpeed = 0.0f;
    float currentSpeed = 0.0f;
    bool speedBoosted = false;
    int_t speedBoostTime = 0;
    int_t maxSpeedBoostTime = 0;
};
