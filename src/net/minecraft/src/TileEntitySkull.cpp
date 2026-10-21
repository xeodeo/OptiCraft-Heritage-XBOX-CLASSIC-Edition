#include "TileEntitySkull.h"

#include "NBTTagCompound.h"

TileEntitySkull::TileEntitySkull()
{
}

void TileEntitySkull::writeToNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::writeToNBT(nbttagcompound);
	nbttagcompound->setByte("SkullType", static_cast<byte_t>(skullType & 0xFF));
	nbttagcompound->setByte("Rot", static_cast<byte_t>(skullRotation & 0xFF));
	nbttagcompound->setString("ExtraType", extraType);
}

void TileEntitySkull::readFromNBT(NBTTagCompound *nbttagcompound)
{
	TileEntity::readFromNBT(nbttagcompound);
	skullType = nbttagcompound->getByte("SkullType");
	skullRotation = nbttagcompound->getByte("Rot");
	if (nbttagcompound->hasKey("ExtraType"))
	{
		extraType = nbttagcompound->getString("ExtraType");
	}
}

void TileEntitySkull::setSkullType(int_t type, const std::string &extra)
{
	skullType = type;
	extraType = extra;
}

int_t TileEntitySkull::getSkullType() const
{
	return skullType;
}

int_t TileEntitySkull::getSkullRotation() const
{
	return skullRotation;
}

void TileEntitySkull::setSkullRotation(int_t rot)
{
	skullRotation = rot;
}

const std::string &TileEntitySkull::getExtraType() const
{
	return extraType;
}
