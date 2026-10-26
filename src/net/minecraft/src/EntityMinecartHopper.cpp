#include "EntityMinecartHopper.h"
#include "Block.h"
#include "EntityItem.h"
#include "EntityPlayer.h"
#include "NBTTagCompound.h"
#include "TileEntityHopper.h"
#include "World.h"

EntityMinecartHopper::EntityMinecartHopper(World *world)
	: EntityMinecart(world),
	  isBlocked(true),
	  transferTicker(-1)
{
	minecartType = 5;
}

EntityMinecartHopper::EntityMinecartHopper(World *world, double x, double y, double z)
	: EntityMinecart(world, x, y, z, 5),
	  isBlocked(true),
	  transferTicker(-1)
{
	minecartType = 5;
}

bool EntityMinecartHopper::interact(EntityPlayer *entityplayer)
{
	if (!worldObj->multiplayerWorld)
	{
		entityplayer->displayGUIHopperMinecart(this);
	}
	return true;
}

void EntityMinecartHopper::onActivatorRailPass(int_t x, int_t y, int_t z, bool powered)
{
	(void)x;
	(void)y;
	(void)z;
	bool active = !powered;
	if (active != getBlocked())
	{
		setBlocked(active);
	}
}

void EntityMinecartHopper::onUpdate()
{
	EntityMinecart::onUpdate();
	if (!worldObj->multiplayerWorld && isEntityAlive() && getBlocked())
	{
		--transferTicker;
		if (!canTransfer())
		{
			setTransferTicker(0);
			if (func_96112_aD())
			{
				setTransferTicker(4);
				onInventoryChanged();
			}
		}
	}
}

bool EntityMinecartHopper::func_96112_aD()
{
	if (TileEntityHopper::suckItemsIntoHopper(this))
	{
		return true;
	}

	const auto &items = worldObj->getEntitiesWithinAABB(
		typeid(EntityItem),
		boundingBox->expand(0.25, 0.0, 0.25)
	);

	if (!items.empty())
	{
		EntityItem *item = dynamic_cast<EntityItem *>(items[0]);
		if (item != nullptr)
		{
			TileEntityHopper::func_96114_a(this, item);
		}
	}

	return false;
}

void EntityMinecartHopper::killMinecart(const DamageSource &source)
{
	EntityMinecart::killMinecart(source);
	if (Block::hopperBlock != nullptr)
	{
		dropItemWithOffset(Block::hopperBlock->blockID, 1, 0.0f);
	}
}

void EntityMinecartHopper::writeEntityToNBT(NBTTagCompound *tag)
{
	EntityMinecart::writeEntityToNBT(tag);
	tag->setInteger("TransferCooldown", transferTicker);
}

void EntityMinecartHopper::readEntityFromNBT(NBTTagCompound *tag)
{
	EntityMinecart::readEntityFromNBT(tag);
	transferTicker = tag->getInteger("TransferCooldown");
}
