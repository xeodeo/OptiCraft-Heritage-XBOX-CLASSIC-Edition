#pragma once

#include "EntityMinecart.h"
#include "Hopper.h"

class DamageSource;
class NBTTagCompound;
class World;

// net.minecraft.src.EntityMinecartHopper
class EntityMinecartHopper : public EntityMinecart, public Hopper
{
public:
	EntityMinecartHopper(World *world);
	EntityMinecartHopper(World *world, double x, double y, double z);

	int_t getMinecartType() const override { return 5; }
	int_t getSizeInventory() override { return 5; }
	bool interact(EntityPlayer *entityplayer) override;
	void onActivatorRailPass(int_t x, int_t y, int_t z, bool powered) override;

	bool getBlocked() const { return isBlocked; }
	void setBlocked(bool b) { isBlocked = b; }

	World *getWorldObj() override { return worldObj; }
	double getXPos() override { return posX; }
	double getYPos() override { return posY; }
	double getZPos() override { return posZ; }
	IInventory *getHopperInventory() override { return this; }

	void onUpdate() override;
	bool func_96112_aD();

	void killMinecart(const DamageSource &source) override;
	void writeEntityToNBT(NBTTagCompound *tag) override;
	void readEntityFromNBT(NBTTagCompound *tag) override;

	void setTransferTicker(int_t t) { transferTicker = t; }
	bool canTransfer() const { return transferTicker > 0; }

private:
	bool isBlocked;
	int_t transferTicker;
};
