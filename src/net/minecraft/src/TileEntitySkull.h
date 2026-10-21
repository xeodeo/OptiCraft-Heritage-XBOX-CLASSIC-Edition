#pragma once

#include "TileEntity.h"
#include <string>

// net.minecraft.src.TileEntitySkull
class TileEntitySkull : public TileEntity
{
public:
	TileEntitySkull();

	void writeToNBT(NBTTagCompound *nbttagcompound) override;
	void readFromNBT(NBTTagCompound *nbttagcompound) override;

	void setSkullType(int_t type, const std::string &extra = "");
	int_t getSkullType() const;
	int_t getSkullRotation() const;
	void setSkullRotation(int_t rot);
	const std::string &getExtraType() const;

private:
	int_t skullType = 0;
	int_t skullRotation = 0;
	std::string extraType;
};
