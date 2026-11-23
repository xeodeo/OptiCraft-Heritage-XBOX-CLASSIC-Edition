#include "DamageSource.h"

#include "Entity.h"
#include "EntityArrow.h"
#include "EntityFireball.h"
#include "EntityLiving.h"
#include "EntityPlayer.h"
#include "Explosion.h"
#include "ItemStack.h"
#include "StatCollector.h"

DamageSource DamageSource::inFire = DamageSource("inFire").setFireDamage();
DamageSource DamageSource::onFire = DamageSource("onFire").setDamageBypassesArmor().setFireDamage();
DamageSource DamageSource::lava = DamageSource("lava").setFireDamage();
DamageSource DamageSource::inWall = DamageSource("inWall").setDamageBypassesArmor();
DamageSource DamageSource::drown = DamageSource("drown").setDamageBypassesArmor();
DamageSource DamageSource::starve = DamageSource("starve").setDamageBypassesArmor();
DamageSource DamageSource::cactus("cactus");
DamageSource DamageSource::fall = DamageSource("fall").setDamageBypassesArmor();
DamageSource DamageSource::outOfWorld = DamageSource("outOfWorld").setDamageBypassesArmor().setDamageAllowedInCreativeMode();
DamageSource DamageSource::generic = DamageSource("generic").setDamageBypassesArmor();
DamageSource DamageSource::explosion = DamageSource("explosion").setDifficultyScaled().setExplosion();
DamageSource DamageSource::magic = DamageSource("magic").setDamageBypassesArmor().setMagicDamage();
DamageSource DamageSource::wither = DamageSource("wither").setDamageBypassesArmor();
DamageSource DamageSource::anvil("anvil");
DamageSource DamageSource::fallingBlock("fallingBlock");

DamageSource::DamageSource(const jstring &type)
	: DamageSource(type, nullptr, nullptr, SourceKind::Generic)
{
}

DamageSource::DamageSource(const jstring &type, Entity *direct, Entity *responsible, SourceKind kind)
	: directSource(direct), responsibleEntity(responsible), sourceKind(kind), unblockable(false),
	  damageAllowedInCreativeMode(false), hungerDamage(0.3f), fire(false), projectile(false),
	  difficultyScaled(false), explosionFlag(false), magicDamage(false), damageType(type)
{
}

DamageSource DamageSource::causeMobDamage(EntityLiving *entity)
{
	return DamageSource("mob", entity, entity, SourceKind::EntityDirect);
}

DamageSource DamageSource::causePlayerDamage(EntityPlayer *player)
{
	return DamageSource("player", player, player, SourceKind::EntityDirect);
}

DamageSource DamageSource::causeArrowDamage(EntityArrow *arrow, Entity *shooter)
{
	DamageSource source("arrow", arrow, shooter, SourceKind::EntityIndirect);
	return source.setProjectile();
}

DamageSource DamageSource::causeFireballDamage(EntityFireball *fireball, Entity *shooter)
{
	if (shooter == nullptr)
	{
		DamageSource source("onFire", fireball, fireball, SourceKind::EntityIndirect);
		return source.setFireDamage().setProjectile();
	}
	DamageSource source("fireball", fireball, shooter, SourceKind::EntityIndirect);
	return source.setFireDamage().setProjectile();
}

DamageSource DamageSource::causeThrownDamage(Entity *projectileEntity, Entity *thrower)
{
	DamageSource source("thrown", projectileEntity, thrower, SourceKind::EntityIndirect);
	return source.setProjectile();
}

DamageSource DamageSource::causeIndirectMagicDamage(Entity *sourceEntity, Entity *indirectEntity)
{
	DamageSource source("indirectMagic", sourceEntity, indirectEntity, SourceKind::EntityIndirect);
	return source.setDamageBypassesArmor().setMagicDamage();
}

DamageSource DamageSource::causeThornsDamage(Entity *entity)
{
	// Java 1.5.2: new EntityDamageSource("thorns", entity).setMagicDamage()
	DamageSource source("thorns", entity, entity, SourceKind::EntityDirect);
	return source.setMagicDamage();
}

DamageSource DamageSource::setExplosionSource(Explosion *explosionObj)
{
	EntityLiving *placedBy = (explosionObj != nullptr) ? explosionObj->func_94613_c() : nullptr;
	if (placedBy != nullptr)
	{
		DamageSource source("explosion.player", placedBy, placedBy, SourceKind::EntityDirect);
		return source.setDifficultyScaled().setExplosion();
	}
	DamageSource source("explosion");
	return source.setDifficultyScaled().setExplosion();
}

bool DamageSource::isProjectile() const
{
	return projectile;
}

DamageSource &DamageSource::setProjectile()
{
	projectile = true;
	return *this;
}

bool DamageSource::isUnblockable() const
{
	return unblockable;
}

float DamageSource::getHungerDamage() const
{
	return hungerDamage;
}

bool DamageSource::canHarmInCreative() const
{
	return damageAllowedInCreativeMode;
}

Entity *DamageSource::getSourceOfDamage() const
{
	return directSource;
}

Entity *DamageSource::getEntity() const
{
	return responsibleEntity;
}

DamageSource &DamageSource::setDamageBypassesArmor()
{
	unblockable = true;
	hungerDamage = 0.0f;
	return *this;
}

DamageSource &DamageSource::setDamageAllowedInCreativeMode()
{
	damageAllowedInCreativeMode = true;
	return *this;
}

DamageSource &DamageSource::setFireDamage()
{
	fire = true;
	return *this;
}

bool DamageSource::fireDamage() const
{
	return fire;
}

bool DamageSource::isExplosion() const
{
	return explosionFlag || damageType == "explosion" || damageType == "explosion.player";
}

DamageSource &DamageSource::setExplosion()
{
	explosionFlag = true;
	return *this;
}

bool DamageSource::isDifficultyScaled() const
{
	if (sourceKind != SourceKind::Generic)
	{
		Entity *src = (sourceKind == SourceKind::EntityIndirect) ? responsibleEntity : directSource;
		if (src != nullptr && src->isLiving() && !src->isPlayer())
			return true;
	}
	return difficultyScaled;
}

DamageSource &DamageSource::setDifficultyScaled()
{
	difficultyScaled = true;
	return *this;
}

bool DamageSource::isMagicDamage() const
{
	return magicDamage;
}

DamageSource &DamageSource::setMagicDamage()
{
	magicDamage = true;
	return *this;
}

const jstring &DamageSource::getDamageType() const
{
	return damageType;
}

std::string DamageSource::getDeathMessage(EntityLiving *victim) const
{
	if (victim == nullptr)
		return "";

	const std::string victimName = victim->getTranslatedEntityName();
	const std::string baseKey = "death.attack." + static_cast<std::string>(damageType);

	if (sourceKind == SourceKind::EntityIndirect)
	{
		Entity *killer = (responsibleEntity != nullptr) ? responsibleEntity : directSource;
		const std::string killerName = (killer != nullptr) ? killer->getTranslatedEntityName() : StatCollector::translateToLocal("entity.generic.name");
		ItemStack *held = (responsibleEntity != nullptr && responsibleEntity->isLiving())
			? static_cast<EntityLiving *>(responsibleEntity)->getHeldItem()
			: nullptr;
		const std::string itemKey = baseKey + ".item";
		if (held != nullptr && held->hasDisplayName() && StatCollector::func_94522_b(itemKey))
			return StatCollector::translateToLocalFormatted(itemKey, victimName, killerName, held->getDisplayName());
		return StatCollector::translateToLocalFormatted(baseKey, victimName, killerName);
	}

	if (sourceKind == SourceKind::EntityDirect)
	{
		Entity *killer = (responsibleEntity != nullptr) ? responsibleEntity : directSource;
		const std::string killerName = (killer != nullptr) ? killer->getTranslatedEntityName() : StatCollector::translateToLocal("entity.generic.name");
		ItemStack *held = (killer != nullptr && killer->isLiving())
			? static_cast<EntityLiving *>(killer)->getHeldItem()
			: nullptr;
		const std::string itemKey = baseKey + ".item";
		if (held != nullptr && held->hasDisplayName() && StatCollector::func_94522_b(itemKey))
			return StatCollector::translateToLocalFormatted(itemKey, victimName, killerName, held->getDisplayName());
		return StatCollector::translateToLocalFormatted(baseKey, victimName, killerName);
	}

	EntityLiving *attacker = victim->func_94060_bK();
	const std::string playerKey = baseKey + ".player";
	if (attacker != nullptr && StatCollector::func_94522_b(playerKey))
		return StatCollector::translateToLocalFormatted(playerKey, victimName, attacker->getTranslatedEntityName());
	return StatCollector::translateToLocalFormatted(baseKey, victimName);
}
