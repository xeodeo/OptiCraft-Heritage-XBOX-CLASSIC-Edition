#pragma once

class EntityLiving;

// net.minecraft.src.IRangedAttackMob
class IRangedAttackMob
{
public:
	virtual ~IRangedAttackMob() = default;
	virtual void attackEntityWithRangedAttack(EntityLiving *target, float distanceFactor) = 0;
};
