#include "TileEntityEnderChest.h"

#include "Block.h"
#include "EntityPlayer.h"
#include "World.h"

TileEntityEnderChest::TileEntityEnderChest()
{
}

void TileEntityEnderChest::updateEntity()
{
	TileEntity::updateEntity();
	if (++ticksSinceSync % 80 == 0 && worldObj != nullptr && Block::enderChest != nullptr)
	{
		worldObj->playNoteAt(xCoord, yCoord, zCoord, 1, numUsingPlayers);
	}
	prevLidAngle = lidAngle;
	const float step = 0.1f;
	if (numUsingPlayers > 0 && lidAngle == 0.0f && worldObj != nullptr)
	{
		worldObj->playSoundEffect((double)xCoord + 0.5, (double)yCoord + 0.5, (double)zCoord + 0.5,
		                          "random.chestopen", 0.5f, worldObj->rand.nextFloat() * 0.1f + 0.9f);
	}
	if ((numUsingPlayers == 0 && lidAngle > 0.0f) || (numUsingPlayers > 0 && lidAngle < 1.0f))
	{
		const float prev = lidAngle;
		if (numUsingPlayers > 0)
		{
			lidAngle += step;
		}
		else
		{
			lidAngle -= step;
		}
		if (lidAngle > 1.0f)
		{
			lidAngle = 1.0f;
		}
		const float half = 0.5f;
		if (lidAngle < half && prev >= half && worldObj != nullptr)
		{
			worldObj->playSoundEffect((double)xCoord + 0.5, (double)yCoord + 0.5, (double)zCoord + 0.5,
			                          "random.chestclosed", 0.5f, worldObj->rand.nextFloat() * 0.1f + 0.9f);
		}
		if (lidAngle < 0.0f)
		{
			lidAngle = 0.0f;
		}
	}
}

void TileEntityEnderChest::onTileEntityPowered(int_t eventId, int_t eventParam)
{
	if (eventId == 1)
	{
		numUsingPlayers = eventParam;
	}
	else
	{
		TileEntity::onTileEntityPowered(eventId, eventParam);
	}
}

void TileEntityEnderChest::invalidate()
{
	updateContainingBlockInfo();
	TileEntity::invalidate();
}

void TileEntityEnderChest::openChest()
{
	++numUsingPlayers;
	if (worldObj != nullptr && Block::enderChest != nullptr)
	{
		worldObj->playNoteAt(xCoord, yCoord, zCoord, 1, numUsingPlayers);
	}
}

void TileEntityEnderChest::closeChest()
{
	--numUsingPlayers;
	if (worldObj != nullptr && Block::enderChest != nullptr)
	{
		worldObj->playNoteAt(xCoord, yCoord, zCoord, 1, numUsingPlayers);
	}
}

bool TileEntityEnderChest::isUseableByPlayer(EntityPlayer *player)
{
	if (worldObj == nullptr || worldObj->getBlockTileEntity(xCoord, yCoord, zCoord) != this)
	{
		return false;
	}
	return player->getDistanceSq((double)xCoord + 0.5, (double)yCoord + 0.5, (double)zCoord + 0.5) <= 64.0;
}
