#include "TileEntityMobSpawner.h"

#include "Block.h"
#include "NBTTagCompound.h"
#include "World.h"

TileEntityMobSpawnerLogic::TileEntityMobSpawnerLogic(TileEntityMobSpawner *owner)
	: mobSpawnerEntity(owner)
{
}

void TileEntityMobSpawnerLogic::broadcastEvent(int_t eventId)
{
	if (mobSpawnerEntity != nullptr && mobSpawnerEntity->worldObj != nullptr && Block::mobSpawner != nullptr)
	{
		mobSpawnerEntity->worldObj->playNoteAt(
			mobSpawnerEntity->xCoord,
			mobSpawnerEntity->yCoord,
			mobSpawnerEntity->zCoord,
			eventId,
			0);
	}
}

World *TileEntityMobSpawnerLogic::getSpawnerWorld()
{
	return mobSpawnerEntity != nullptr ? mobSpawnerEntity->worldObj : nullptr;
}

int_t TileEntityMobSpawnerLogic::getSpawnerX()
{
	return mobSpawnerEntity != nullptr ? mobSpawnerEntity->xCoord : 0;
}

int_t TileEntityMobSpawnerLogic::getSpawnerY()
{
	return mobSpawnerEntity != nullptr ? mobSpawnerEntity->yCoord : 0;
}

int_t TileEntityMobSpawnerLogic::getSpawnerZ()
{
	return mobSpawnerEntity != nullptr ? mobSpawnerEntity->zCoord : 0;
}

TileEntityMobSpawner::TileEntityMobSpawner()
	: spawnerLogic(this)
{
}

std::string TileEntityMobSpawner::getMobID()
{
	return spawnerLogic.getEntityNameToSpawn();
}

void TileEntityMobSpawner::setMobID(const std::string &s)
{
	spawnerLogic.setMobID(s);
}

bool TileEntityMobSpawner::anyPlayerInRange()
{
	return spawnerLogic.canRun();
}

void TileEntityMobSpawner::readFromNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::readFromNBT(nbttagcompound);
	spawnerLogic.readFromNBT(nbttagcompound);
}

void TileEntityMobSpawner::writeToNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::writeToNBT(nbttagcompound);
	spawnerLogic.writeToNBT(nbttagcompound);
}

void TileEntityMobSpawner::updateEntity()
{
	spawnerLogic.updateSpawner();
	TileEntity::updateEntity();
}
