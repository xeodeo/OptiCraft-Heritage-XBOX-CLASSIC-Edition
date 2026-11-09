#include "EntityWither.h"

#include <cmath>
#include <vector>

#include "AxisAlignedBB.h"
#include "Block.h"
#include "DamageSource.h"
#include "DataWatcher.h"
#include "EntityAIArrowAttack.h"
#include "EntityAIHurtByTarget.h"
#include "EntityAILookIdle.h"
#include "EntityAINearestAttackableTarget.h"
#include "EntityAISwimming.h"
#include "EntityAIWander.h"
#include "EntityAIWatchClosest.h"
#include "EntityArrow.h"
#include "EntityPlayer.h"
#include "EntityWitherSkull.h"
#include "Item.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "PathNavigate.h"
#include "PotionEffect.h"
#include "World.h"

namespace
{
constexpr float kPi = 3.1415927f;

double randomInRange(Random &random, double min, double max)
{
	return min >= max ? min : random.nextDouble() * (max - min) + min;
}
}

bool EntityWither::attackEntitySelector(Entity *entity)
{
	EntityLiving *living = dynamic_cast<EntityLiving *>(entity);
	return living != nullptr && living->getCreatureAttribute() != EnumCreatureAttribute::UNDEAD;
}

EntityWither::EntityWither(World *world)
	: EntityMob(world), field_82222_j(0)
{
	ensureEntityInit();
	for (int_t i = 0; i < 2; ++i)
	{
		field_82220_d[i] = field_82221_e[i] = field_82217_f[i] = field_82218_g[i] = 0.0f;
		field_82223_h[i] = field_82224_i[i] = 0;
	}
	setEntityHealth(getMaxHealth());
	texture = "/mob/wither.png";
	setSize(0.9f, 4.0f);
	immuneToFire = true;
	moveSpeed = 0.6f;
	if (getNavigator() != nullptr)
		getNavigator()->setCanSwim(true);
	tasks.addTask(0, new EntityAISwimming(this));
	tasks.addTask(2, new EntityAIArrowAttack(this, moveSpeed, 40, 20.0f));
	tasks.addTask(5, new EntityAIWander(this, moveSpeed));
	tasks.addTask(6, new EntityAIWatchClosest(this, typeid(EntityPlayer), 8.0f));
	tasks.addTask(7, new EntityAILookIdle(this));
	targetTasks.addTask(1, new EntityAIHurtByTarget(this, false));
	targetTasks.addTask(2, new EntityAINearestAttackableTarget(this, typeid(EntityLiving), 30.0f, 0, false, false,
		&EntityWither::attackEntitySelector));
	experienceValue = 50;
}

void EntityWither::entityInit()
{
	EntityMob::entityInit();
	dataWatcher->addObject(16, static_cast<int_t>(100));
	dataWatcher->addObject(17, static_cast<int_t>(0));
	dataWatcher->addObject(18, static_cast<int_t>(0));
	dataWatcher->addObject(19, static_cast<int_t>(0));
	dataWatcher->addObject(20, static_cast<int_t>(0));
}

void EntityWither::writeEntityToNBT(NBTTagCompound *nbttagcompound)
{
	EntityMob::writeEntityToNBT(nbttagcompound);
	nbttagcompound->setInteger("Invul", func_82212_n());
}

void EntityWither::readEntityFromNBT(NBTTagCompound *nbttagcompound)
{
	EntityMob::readEntityFromNBT(nbttagcompound);
	func_82215_s(nbttagcompound->getInteger("Invul"));
	dataWatcher->updateObject(16, health);
}

float EntityWither::getShadowSize()
{
	return height / 8.0f;
}

jstring EntityWither::getLivingSound() { return "mob.wither.idle"; }
jstring EntityWither::getHurtSound() { return "mob.wither.hurt"; }
jstring EntityWither::getDeathSound() { return "mob.wither.death"; }

const char *EntityWither::getEntityTexture()
{
	const int_t invul = func_82212_n();
	return invul > 0 && (invul > 80 || invul / 5 % 2 != 1) ? "/mob/wither_invul.png" : "/mob/wither.png";
}

void EntityWither::onLivingUpdate()
{
	if (!worldObj->multiplayerWorld)
		dataWatcher->updateObject(16, health);

	motionY *= static_cast<double>(0.6f);
	if (!worldObj->multiplayerWorld && getWatchedTargetId(0) > 0)
	{
		Entity *target = worldObj->getEntityByID(getWatchedTargetId(0));
		if (target != nullptr)
		{
			if (posY < target->posY || (!isArmored() && posY < target->posY + 5.0))
			{
				if (motionY < 0.0)
					motionY = 0.0;
				motionY += (0.5 - motionY) * static_cast<double>(0.6f);
			}

			const double dx = target->posX - posX;
			const double dz = target->posZ - posZ;
			const double distSq = dx * dx + dz * dz;
			if (distSq > 9.0)
			{
				const double dist = static_cast<double>(MathHelper::sqrt_double(distSq));
				motionX += (dx / dist * 0.5 - motionX) * static_cast<double>(0.6f);
				motionZ += (dz / dist * 0.5 - motionZ) * static_cast<double>(0.6f);
			}
		}
	}

	if (motionX * motionX + motionZ * motionZ > static_cast<double>(0.05f))
		rotationYaw = static_cast<float>(std::atan2(motionZ, motionX)) * (180.0f / kPi) - 90.0f;

	EntityMob::onLivingUpdate();

	for (int_t i = 0; i < 2; ++i)
	{
		field_82218_g[i] = field_82221_e[i];
		field_82217_f[i] = field_82220_d[i];
	}

	for (int_t i = 0; i < 2; ++i)
	{
		const int_t targetId = getWatchedTargetId(i + 1);
		Entity *target = targetId > 0 ? worldObj->getEntityByID(targetId) : nullptr;
		if (target != nullptr)
		{
			const double headX = func_82214_u(i + 1);
			const double headY = func_82208_v(i + 1);
			const double headZ = func_82213_w(i + 1);
			const double dx = target->posX - headX;
			const double dy = target->posY + static_cast<double>(target->getEyeHeight()) - headY;
			const double dz = target->posZ - headZ;
			const double horizontal = static_cast<double>(MathHelper::sqrt_double(dx * dx + dz * dz));
			const float yaw = static_cast<float>(std::atan2(dz, dx) * 180.0 / static_cast<double>(kPi)) - 90.0f;
			const float pitch = static_cast<float>(-(std::atan2(dy, horizontal) * 180.0 / static_cast<double>(kPi)));
			field_82220_d[i] = func_82204_b(field_82220_d[i], pitch, 40.0f);
			field_82221_e[i] = func_82204_b(field_82221_e[i], yaw, 10.0f);
		}
		else
		{
			field_82221_e[i] = func_82204_b(field_82221_e[i], renderYawOffset, 10.0f);
		}
	}

	const bool armored = isArmored();
	for (int_t head = 0; head < 3; ++head)
	{
		const double hx = func_82214_u(head);
		const double hy = func_82208_v(head);
		const double hz = func_82213_w(head);
		worldObj->spawnParticle("smoke", hx + rand.nextGaussian() * static_cast<double>(0.3f),
			hy + rand.nextGaussian() * static_cast<double>(0.3f), hz + rand.nextGaussian() * static_cast<double>(0.3f),
			0.0, 0.0, 0.0);
		if (armored && worldObj->rand.nextInt(4) == 0)
		{
			worldObj->spawnParticle("mobSpell", hx + rand.nextGaussian() * static_cast<double>(0.3f),
				hy + rand.nextGaussian() * static_cast<double>(0.3f), hz + rand.nextGaussian() * static_cast<double>(0.3f),
				static_cast<double>(0.7f), static_cast<double>(0.7f), 0.5);
		}
	}

	if (func_82212_n() > 0)
	{
		for (int_t i = 0; i < 3; ++i)
		{
			worldObj->spawnParticle("mobSpell", posX + rand.nextGaussian(),
				posY + static_cast<double>(rand.nextFloat() * 3.3f), posZ + rand.nextGaussian(),
				static_cast<double>(0.7f), static_cast<double>(0.7f), static_cast<double>(0.9f));
		}
	}
}

void EntityWither::updateAITasks()
{
	if (func_82212_n() > 0)
	{
		const int_t remaining = func_82212_n() - 1;
		if (remaining <= 0)
		{
			delete worldObj->newExplosion(this, posX, posY + static_cast<double>(getEyeHeight()), posZ, 7.0f, false);
			worldObj->playAuxSFX(1013, static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ), 0);
		}
		func_82215_s(remaining);
		if (ticksExisted % 10 == 0)
			heal(10);
		return;
	}

	EntityMob::updateAITasks();

	for (int_t head = 1; head < 3; ++head)
	{
		if (ticksExisted < field_82223_h[head - 1])
			continue;

		field_82223_h[head - 1] = ticksExisted + 10 + rand.nextInt(10);
		if (worldObj->difficultySetting >= 2 && field_82224_i[head - 1]++ > 15)
		{
			const float spreadXZ = 10.0f;
			const float spreadY = 5.0f;
			const double x = randomInRange(rand, posX - static_cast<double>(spreadXZ), posX + static_cast<double>(spreadXZ));
			const double y = randomInRange(rand, posY - static_cast<double>(spreadY), posY + static_cast<double>(spreadY));
			const double z = randomInRange(rand, posZ - static_cast<double>(spreadXZ), posZ + static_cast<double>(spreadXZ));
			func_82209_a(head + 1, x, y, z, true);
			field_82224_i[head - 1] = 0;
		}

		const int_t targetId = getWatchedTargetId(head);
		if (targetId > 0)
		{
			Entity *target = worldObj->getEntityByID(targetId);
			EntityLiving *living = dynamic_cast<EntityLiving *>(target);
			if (living != nullptr && living->isEntityAlive() && !(getDistanceSqToEntity(living) > 900.0) &&
				canEntityBeSeen(living))
			{
				func_82216_a(head + 1, living);
				field_82223_h[head - 1] = ticksExisted + 40 + rand.nextInt(20);
				field_82224_i[head - 1] = 0;
			}
			else
			{
				func_82211_c(head, 0);
			}
		}
		else
		{
			std::vector<Entity *> candidates = worldObj->getEntitiesWithinAABB(typeid(EntityLiving),
				boundingBox->expand(20.0, 8.0, 20.0));
			for (std::size_t i = 0; i < candidates.size();)
			{
				if (attackEntitySelector(candidates[i]))
					++i;
				else
					candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(i));
			}

			for (int_t tries = 0; tries < 10 && !candidates.empty(); ++tries)
			{
				const int_t pick = rand.nextInt(static_cast<int_t>(candidates.size()));
				EntityLiving *living = static_cast<EntityLiving *>(candidates[static_cast<std::size_t>(pick)]);
				if (living != this && living->isEntityAlive() && canEntityBeSeen(living))
				{
					EntityPlayer *player = dynamic_cast<EntityPlayer *>(living);
					if (player == nullptr || !player->capabilities.disableDamage)
						func_82211_c(head, living->entityId);
					break;
				}
				candidates.erase(candidates.begin() + pick);
			}
		}
	}

	EntityLiving *attackTarget = getAttackTarget();
	func_82211_c(0, attackTarget != nullptr ? attackTarget->entityId : 0);

	if (field_82222_j > 0)
	{
		--field_82222_j;
		if (field_82222_j == 0)
			destroyBlocksAround();
	}

	if (ticksExisted % 20 == 0)
		heal(1);
}

// Java World.destroyBlock(x, y, z, true) for every block of the 3x4x3 box around the Wither
// (mobGriefing is always on: this port has no game rules yet).
void EntityWither::destroyBlocksAround()
{
	const int_t baseY = MathHelper::floor_double(posY);
	const int_t baseX = MathHelper::floor_double(posX);
	const int_t baseZ = MathHelper::floor_double(posZ);
	bool destroyed = false;
	for (int_t dx = -1; dx <= 1; ++dx)
	{
		for (int_t dz = -1; dz <= 1; ++dz)
		{
			for (int_t dy = 0; dy <= 3; ++dy)
			{
				const int_t x = baseX + dx;
				const int_t y = baseY + dy;
				const int_t z = baseZ + dz;
				const int_t id = worldObj->getBlockId(x, y, z);
				if (id <= 0 || id == Block::bedrock->blockID || id == Block::endPortal->blockID ||
					id == Block::endPortalFrame->blockID)
					continue;
				Block *block = Block::blocksList[id];
				if (block == nullptr)
					continue;
				const int_t meta = worldObj->getBlockMetadata(x, y, z);
				worldObj->playAuxSFX(2001, x, y, z, id + (meta << 12));
				block->dropBlockAsItem(worldObj, x, y, z, meta, 0);
				destroyed = worldObj->setBlockNotify152(x, y, z, 0, 0, 3) || destroyed;
			}
		}
	}
	if (destroyed)
		worldObj->playAuxSFXAtEntity(nullptr, 1012, static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ), 0);
}

void EntityWither::func_82206_m()
{
	func_82215_s(220);
	setEntityHealth(getMaxHealth() / 3);
}

void EntityWither::setInWeb()
{
}

int_t EntityWither::getTotalArmorValue() const
{
	return 4;
}

double EntityWither::func_82214_u(int_t head) const
{
	if (head <= 0)
		return posX;
	const float angle = (renderYawOffset + static_cast<float>(180 * (head - 1))) / 180.0f * kPi;
	return posX + static_cast<double>(MathHelper::cos(angle)) * 1.3;
}

double EntityWither::func_82208_v(int_t head) const
{
	return head <= 0 ? posY + 3.0 : posY + 2.2;
}

double EntityWither::func_82213_w(int_t head) const
{
	if (head <= 0)
		return posZ;
	const float angle = (renderYawOffset + static_cast<float>(180 * (head - 1))) / 180.0f * kPi;
	return posZ + static_cast<double>(MathHelper::sin(angle)) * 1.3;
}

float EntityWither::func_82204_b(float current, float target, float maxStep) const
{
	float delta = MathHelper::wrapAngleTo180_float(target - current);
	if (delta > maxStep)
		delta = maxStep;
	if (delta < -maxStep)
		delta = -maxStep;
	return current + delta;
}

void EntityWither::func_82216_a(int_t head, EntityLiving *target)
{
	func_82209_a(head, target->posX, target->posY + static_cast<double>(target->getEyeHeight()) * 0.5, target->posZ,
		head == 0 && rand.nextFloat() < 0.001f);
}

void EntityWither::func_82209_a(int_t head, double x, double y, double z, bool invulnerableSkull)
{
	worldObj->playAuxSFXAtEntity(nullptr, 1014, static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ), 0);
	const double headX = func_82214_u(head);
	const double headY = func_82208_v(head);
	const double headZ = func_82213_w(head);
	EntityWitherSkull *skull = new EntityWitherSkull(worldObj, this, x - headX, y - headY, z - headZ);
	if (invulnerableSkull)
		skull->setInvulnerable(true);
	skull->posY = headY;
	skull->posX = headX;
	skull->posZ = headZ;
	worldObj->spawnEntityInWorld(skull);
}

void EntityWither::attackEntityWithRangedAttack(EntityLiving *target, float distanceFactor)
{
	(void)distanceFactor;
	if (target != nullptr)
		func_82216_a(0, target);
}

bool EntityWither::attackEntityFrom(const DamageSource &source, int_t damage)
{
	if (isEntityInvulnerable())
		return false;
	if (source.getDamageType() == DamageSource::drown.getDamageType())
		return false;
	if (func_82212_n() > 0)
		return false;

	if (isArmored() && dynamic_cast<EntityArrow *>(source.getSourceOfDamage()) != nullptr)
		return false;

	Entity *attacker = source.getEntity();
	EntityLiving *livingAttacker = dynamic_cast<EntityLiving *>(attacker);
	if (livingAttacker != nullptr && dynamic_cast<EntityPlayer *>(attacker) == nullptr &&
		livingAttacker->getCreatureAttribute() == getCreatureAttribute())
		return false;

	if (field_82222_j <= 0)
		field_82222_j = 20;
	for (int_t i = 0; i < 2; ++i)
		field_82224_i[i] += 3;
	return EntityMob::attackEntityFrom(source, damage);
}

void EntityWither::dropFewItems(bool recentlyHitByPlayer, int_t lootingLevel)
{
	(void)recentlyHitByPlayer;
	(void)lootingLevel;
	if (Item::netherStar != nullptr)
		dropItem(Item::netherStar->shiftedIndex, 1);
}

void EntityWither::despawnEntity()
{
	entityAge = 0;
}

int_t EntityWither::getBrightnessForRender(float partialTick)
{
	(void)partialTick;
	return 15728880;
}

bool EntityWither::canBeCollidedWith()
{
	return !isDead;
}

int_t EntityWither::getBossHealth() const
{
	return dataWatcher->getWatchableObjectInt(16);
}

void EntityWither::fall(float distance)
{
	(void)distance;
}

void EntityWither::addPotionEffect(PotionEffect *effect)
{
	// Immune to potion effects; the caller hands over ownership.
	delete effect;
}

bool EntityWither::isAIEnabled()
{
	return true;
}

int_t EntityWither::getMaxHealth() const
{
	return 300;
}

float EntityWither::func_82207_a(int_t head) const
{
	return field_82221_e[head];
}

float EntityWither::func_82210_r(int_t head) const
{
	return field_82220_d[head];
}

int_t EntityWither::func_82212_n() const
{
	return dataWatcher->getWatchableObjectInt(20);
}

void EntityWither::func_82215_s(int_t ticks)
{
	dataWatcher->updateObject(20, ticks);
}

int_t EntityWither::getWatchedTargetId(int_t head) const
{
	return dataWatcher->getWatchableObjectInt(17 + head);
}

void EntityWither::func_82211_c(int_t head, int_t entityId)
{
	dataWatcher->updateObject(17 + head, entityId);
}

bool EntityWither::isArmored() const
{
	return getBossHealth() <= getMaxHealth() / 2;
}

EnumCreatureAttribute EntityWither::getCreatureAttribute() const
{
	return EnumCreatureAttribute::UNDEAD;
}

void EntityWither::mountEntity(Entity *entity)
{
	(void)entity;
	ridingEntity = nullptr;
}
