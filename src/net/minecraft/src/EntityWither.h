#pragma once

#include "EntityMob.h"
#include "IRangedAttackMob.h"

class World;
class Entity;
class NBTTagCompound;
class DamageSource;
class PotionEffect;

// net.minecraft.src.EntityWither (Java 1.5.2): the three-headed boss.
class EntityWither : public EntityMob, public IRangedAttackMob
{
public:
	explicit EntityWither(World *world);

	void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;
	void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;
	float getShadowSize() override;
	const char *getEntityTexture() override;
	void onLivingUpdate() override;
	void setInWeb() override;
	int_t getTotalArmorValue() const override;
	void attackEntityWithRangedAttack(EntityLiving *target, float distanceFactor) override;
	bool attackEntityFrom(const DamageSource &source, int_t damage) override;
	int_t getBrightnessForRender(float partialTick) override;
	bool canBeCollidedWith() override;
	void addPotionEffect(PotionEffect *effect) override;
	bool isAIEnabled() override;
	int_t getMaxHealth() const override;
	EnumCreatureAttribute getCreatureAttribute() const override;
	void mountEntity(Entity *entity) override;

	void func_82206_m();                       // freshly summoned: invulnerable, charging, 1/3 health
	int_t getBossHealth() const;
	float func_82207_a(int_t head) const;      // head yaw
	float func_82210_r(int_t head) const;      // head pitch
	int_t func_82212_n() const;                // invulnerable ticks left
	void func_82215_s(int_t ticks);
	int_t getWatchedTargetId(int_t head) const;
	void func_82211_c(int_t head, int_t entityId);
	bool isArmored() const;

	static bool attackEntitySelector(Entity *entity);

protected:
	void entityInit() override;
	jstring getLivingSound() override;
	jstring getHurtSound() override;
	jstring getDeathSound() override;
	void updateAITasks() override;
	void dropFewItems(bool recentlyHitByPlayer, int_t lootingLevel) override;
	void despawnEntity() override;
	void fall(float distance) override;

private:
	double func_82214_u(int_t head) const;
	double func_82208_v(int_t head) const;
	double func_82213_w(int_t head) const;
	float func_82204_b(float current, float target, float maxStep) const;
	void func_82216_a(int_t head, EntityLiving *target);
	void func_82209_a(int_t head, double x, double y, double z, bool invulnerableSkull);
	void destroyBlocksAround();

	float field_82220_d[2];   // head pitch
	float field_82221_e[2];   // head yaw
	float field_82217_f[2];   // previous pitch
	float field_82218_g[2];   // previous yaw
	int_t field_82223_h[2];   // next attack tick per side head
	int_t field_82224_i[2];   // idle counter per side head (random skulls)
	int_t field_82222_j;      // ticks until it breaks the blocks around it after a hit
};
