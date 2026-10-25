#pragma once

#include "EntityMob.h"
#include "IRangedAttackMob.h"

class World;
class Entity;
class NBTTagCompound;
class ItemStack;
class DamageSource;
class EntityAIArrowAttack;
class EntityAIAttackOnCollide;

// net.minecraft.src.EntitySkeleton
class EntitySkeleton : public EntityMob, public IRangedAttackMob
{
public:
	explicit EntitySkeleton(World *world);
	~EntitySkeleton() override;

	static constexpr int_t CLASS_ID = 7;
	int_t getEntityClassID() const override { return CLASS_ID; }

	void onLivingUpdate() override;
	void onDeath(const DamageSource &source) override;
	bool isAIEnabled() override;
	int_t getMaxHealth() const override;

	EnumCreatureAttribute getCreatureAttribute() const override;
	const char *getEntityTexture() override;

	void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;
	void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;

	void initCreature() override;
	void setCombatTask();
	void attackEntityWithRangedAttack(EntityLiving *target, float distanceFactor) override;
	bool attackEntityAsMob(Entity *entity) override;

	int_t getSkeletonType() const;
	void setSkeletonType(int_t type);

	ItemStack *getHeldItem() override;

protected:
	void entityInit() override;
	jstring getLivingSound() override;
	jstring getHurtSound() override;
	jstring getDeathSound() override;
	int_t getDropItemId() override;
	void dropFewItems(bool recentlyHitByPlayer, int_t lootingLevel) override;
	void dropRareDrop(int_t lootingRoll) override;

private:
	EntityAIArrowAttack *aiArrowAttack;
	EntityAIAttackOnCollide *aiAttackOnCollide;
	ItemStack *heldItemInstance;
	static ItemStack *defaultHeldItem;
};
