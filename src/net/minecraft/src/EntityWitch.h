#pragma once

#include "EntityMob.h"
#include "IRangedAttackMob.h"
#include "ItemStack.h"

class World;
class EntityLiving;
class DamageSource;

// net.minecraft.src.EntityWitch
class EntityWitch : public EntityMob, public IRangedAttackMob
{
public:
	explicit EntityWitch(World *world);
	~EntityWitch() override;

	void entityInit() override;
	int_t getMaxHealth() const override { return 26; }
	bool isAIEnabled() override { return true; }
	float getSpeedModifier() override;

	void setAggressive(bool flag);
	bool getAggressive() const;

	void onLivingUpdate() override;
	void handleHealthUpdate(byte_t byte0) override;

	void attackEntityWithRangedAttack(EntityLiving *target, float distanceFactor) override;

	ItemStack *getHeldItem() override;
	const char *getEntityTexture() override { return "/mob/villager/witch.png"; }

protected:
	jstring getLivingSound() override { return "mob.witch.idle"; }
	jstring getHurtSound() override { return "mob.witch.hurt"; }
	jstring getDeathSound() override { return "mob.witch.death"; }

	int_t applyPotionDamageCalculations(const DamageSource &source, int_t damage) override;
	void dropFewItems(bool recentlyHitByPlayer, int_t lootingLevel) override;

private:
	int_t witchAttackTimer = 0;
	ItemStack *heldPotion = nullptr;
};
