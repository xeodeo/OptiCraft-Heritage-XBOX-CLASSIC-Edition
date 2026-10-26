#include "EntityBat.h"

#include <cmath>
#include <ctime>

#include "AxisAlignedBB.h"
#include "DamageSource.h"
#include "DataWatcher.h"
#include "EntityPlayer.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "World.h"

namespace
{
static bool isHalloween()
{
	time_t t = time(nullptr);
	tm *now = localtime(&t);
	if (now == nullptr)
		return false;
	// October 20 to November 3
	return (now->tm_mon == 9 && now->tm_mday >= 20) || (now->tm_mon == 10 && now->tm_mday <= 3);
}
}

EntityBat::EntityBat(World *world)
	: EntityAmbientCreature(world)
{
	ensureEntityInit();
	texture = "/mob/bat.png";
	setSize(0.5f, 0.9f);
	setIsBatHanging(true);
}

void EntityBat::entityInit()
{
	EntityAmbientCreature::entityInit();
	dataWatcher->addObject(16, static_cast<byte_t>(0));
}

int_t EntityBat::getMaxHealth() const
{
	return 6;
}

float EntityBat::getSoundVolume()
{
	return 0.1f;
}

float EntityBat::getSoundPitch()
{
	return EntityAmbientCreature::getSoundPitch() * 0.95f;
}

jstring EntityBat::getLivingSound()
{
	return (getIsBatHanging() && rand.nextInt(4) != 0) ? jstring() : "mob.bat.idle";
}

jstring EntityBat::getHurtSound()
{
	return "mob.bat.hurt";
}

jstring EntityBat::getDeathSound()
{
	return "mob.bat.death";
}

bool EntityBat::canBePushed()
{
	return false;
}

void EntityBat::applyEntityCollision(Entity *)
{
}

bool EntityBat::doesEntityNotTriggerPressurePlate()
{
	return true;
}

bool EntityBat::isAIEnabled()
{
	return true;
}

bool EntityBat::getIsBatHanging() const
{
	return (dataWatcher->getWatchableObjectByte(16) & 1) != 0;
}

void EntityBat::setIsBatHanging(bool hanging)
{
	const byte_t current = dataWatcher->getWatchableObjectByte(16);
	if (hanging)
		dataWatcher->updateObject(16, static_cast<byte_t>(current | 1));
	else
		dataWatcher->updateObject(16, static_cast<byte_t>(current & ~1));
}

const char *EntityBat::getEntityTexture()
{
	return "/mob/bat.png";
}

void EntityBat::onUpdate()
{
	EntityAmbientCreature::onUpdate();
	if (getIsBatHanging())
	{
		motionX = motionY = motionZ = 0.0;
		posY = static_cast<double>(MathHelper::floor_double(posY)) + 1.0 - static_cast<double>(height);
	}
	else
	{
		motionY *= 0.6;
	}
}

void EntityBat::updateAITasks()
{
	EntityAmbientCreature::updateAITasks();
	if (getIsBatHanging())
	{
		if (!worldObj->isBlockNormalCube(MathHelper::floor_double(posX), static_cast<int_t>(posY) + 1, MathHelper::floor_double(posZ)))
		{
			setIsBatHanging(false);
			worldObj->playAuxSFXAtEntity(nullptr, 1015, static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ), 0);
		}
		else
		{
			if (rand.nextInt(200) == 0)
			{
				rotationYawHead = static_cast<float>(rand.nextInt(360));
			}

			if (worldObj->getClosestPlayerToEntity(this, 4.0) != nullptr)
			{
				setIsBatHanging(false);
				worldObj->playAuxSFXAtEntity(nullptr, 1015, static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ), 0);
			}
		}
	}
	else
	{
		if (hasFlightTarget && (!worldObj->isAirBlock(currentFlightTarget.x, currentFlightTarget.y, currentFlightTarget.z) || currentFlightTarget.y < 1))
		{
			hasFlightTarget = false;
		}

		if (!hasFlightTarget || rand.nextInt(30) == 0 ||
			currentFlightTarget.getDistanceSquared(static_cast<int_t>(posX), static_cast<int_t>(posY), static_cast<int_t>(posZ)) < 4.0f)
		{
			currentFlightTarget.set(
				static_cast<int_t>(posX) + rand.nextInt(7) - rand.nextInt(7),
				static_cast<int_t>(posY) + rand.nextInt(6) - 2,
				static_cast<int_t>(posZ) + rand.nextInt(7) - rand.nextInt(7)
			);
			hasFlightTarget = true;
		}

		const double dx = static_cast<double>(currentFlightTarget.x) + 0.5 - posX;
		const double dy = static_cast<double>(currentFlightTarget.y) + 0.1 - posY;
		const double dz = static_cast<double>(currentFlightTarget.z) + 0.5 - posZ;
		auto signum = [](double val) -> double { return (val > 0.0) ? 1.0 : ((val < 0.0) ? -1.0 : 0.0); };
		motionX += (signum(dx) * 0.5 - motionX) * 0.1;
		motionY += (signum(dy) * 0.7 - motionY) * 0.1;
		motionZ += (signum(dz) * 0.5 - motionZ) * 0.1;

		const float yawTarget = static_cast<float>(std::atan2(motionZ, motionX) * 180.0 / 3.141592653589793) - 90.0f;
		const float yawDiff = MathHelper::wrapAngleTo180_float(yawTarget - rotationYaw);
		moveForward = 0.5f;
		rotationYaw += yawDiff;

		if (rand.nextInt(100) == 0 &&
			worldObj->isBlockNormalCube(MathHelper::floor_double(posX), static_cast<int_t>(posY) + 1, MathHelper::floor_double(posZ)))
		{
			setIsBatHanging(true);
		}
	}
}

bool EntityBat::canTriggerWalking()
{
	return false;
}

void EntityBat::fall(float)
{
}

void EntityBat::updateFallState(double, bool)
{
}

bool EntityBat::attackEntityFrom(const DamageSource &source, int_t damage)
{
	if (!worldObj->multiplayerWorld && getIsBatHanging())
	{
		setIsBatHanging(false);
	}

	return EntityAmbientCreature::attackEntityFrom(source, damage);
}

void EntityBat::readEntityFromNBT(NBTTagCompound *nbttagcompound)
{
	EntityAmbientCreature::readEntityFromNBT(nbttagcompound);
	dataWatcher->updateObject(16, nbttagcompound->getByte("BatFlags"));
}

void EntityBat::writeEntityToNBT(NBTTagCompound *nbttagcompound)
{
	EntityAmbientCreature::writeEntityToNBT(nbttagcompound);
	nbttagcompound->setByte("BatFlags", dataWatcher->getWatchableObjectByte(16));
}

bool EntityBat::getCanSpawnHere()
{
	const int_t minY = MathHelper::floor_double(boundingBox->minY);
	if (minY >= 63)
		return false;

	const int_t bx = MathHelper::floor_double(posX);
	const int_t bz = MathHelper::floor_double(posZ);
	const int_t light = worldObj->getBlockLightValue(bx, minY, bz);
	int_t maxLight = 4;

	if (!isHalloween())
	{
		if (rand.nextBoolean())
			return false;
	}
	else
	{
		maxLight = 7;
	}

	if (light > rand.nextInt(maxLight))
		return false;

	return EntityAmbientCreature::getCanSpawnHere();
}

void EntityBat::initCreature()
{
}
