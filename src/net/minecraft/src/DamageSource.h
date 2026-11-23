#pragma once

#include "java/String.h"

class Entity;
class EntityLiving;
class EntityPlayer;
class EntityArrow;
class EntityFireball;
class Explosion;

// net.minecraft.src.DamageSource
class DamageSource
{
public:
	enum class SourceKind
	{
		Generic = 0,
		EntityDirect = 1,
		EntityIndirect = 2
	};

	explicit DamageSource(const jstring &type);
	virtual ~DamageSource() = default;

	static DamageSource causeMobDamage(EntityLiving *entity);
	static DamageSource causePlayerDamage(EntityPlayer *player);
	static DamageSource causeArrowDamage(EntityArrow *arrow, Entity *shooter);
	static DamageSource causeFireballDamage(EntityFireball *fireball, Entity *shooter);
	static DamageSource causeThrownDamage(Entity *projectile, Entity *thrower);
	static DamageSource causeIndirectMagicDamage(Entity *source, Entity *indirectEntity);
	static DamageSource causeThornsDamage(Entity *entity);
	static DamageSource setExplosionSource(Explosion *explosion);

	bool isProjectile() const;
	DamageSource &setProjectile();
	bool isUnblockable() const;
	float getHungerDamage() const;
	bool canHarmInCreative() const;
	virtual Entity *getSourceOfDamage() const;
	virtual Entity *getEntity() const;
	DamageSource &setDamageBypassesArmor();
	DamageSource &setDamageAllowedInCreativeMode();
	DamageSource &setFireDamage();
	bool fireDamage() const;
	bool isFireDamage() const { return fireDamage(); }
	bool isExplosion() const;
	DamageSource &setExplosion();
	bool isDifficultyScaled() const;
	DamageSource &setDifficultyScaled();
	bool isMagicDamage() const;
	DamageSource &setMagicDamage();
	const jstring &getDamageType() const;
	std::string getDeathMessage(EntityLiving *victim) const;

	static DamageSource inFire;
	static DamageSource onFire;
	static DamageSource lava;
	static DamageSource inWall;
	static DamageSource drown;
	static DamageSource starve;
	static DamageSource cactus;
	static DamageSource fall;
	static DamageSource outOfWorld;
	static DamageSource generic;
	static DamageSource explosion;
	static DamageSource magic;
	static DamageSource wither;
	static DamageSource anvil;
	static DamageSource fallingBlock;

protected:
	DamageSource(const jstring &type, Entity *directSource, Entity *responsibleEntity,
	             SourceKind kind = SourceKind::Generic);

	Entity *directSource;
	Entity *responsibleEntity;

private:
	SourceKind sourceKind;
	bool unblockable;
	bool damageAllowedInCreativeMode;
	float hungerDamage;
	bool fire;
	bool projectile;
	bool difficultyScaled;
	bool explosionFlag;
	bool magicDamage = false;
	jstring damageType;
};
