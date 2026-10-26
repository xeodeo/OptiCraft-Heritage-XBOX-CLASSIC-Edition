#pragma once

#include "EntityPlayer.h"

class IInventory;
class Minecraft;
class MovementInput;
class NBTTagCompound;
class Session;
class StatBase;
class TileEntitySign;
class TileEntityFurnace;
class TileEntityDispenser;
class TileEntityBrewingStand;
class IMerchant;
class MerchantRecipeList;

// net.minecraft.src.EntityPlayerSP
class EntityPlayerSP : public EntityPlayer
{
public:
	EntityPlayerSP(Minecraft *minecraft, World *world, Session *session, int_t i);
	~EntityPlayerSP() override;

	void moveEntity(double d, double d1, double d2) override;
	void updatePlayerActionState() override;
	void onLivingUpdate() override;
	void setSprinting(bool value) override;
	float getFOVMultiplier();
	void resetPlayerKeyState();
	void handleKeyPress(int_t i, bool flag);
	void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;
	void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;
	void closeScreen() override;
	void displayGUIEditSign(TileEntitySign *tileentitysign) override;
	void displayGUIHopper(TileEntityHopper *tileentityhopper) override;
	void displayGUIHopperMinecart(EntityMinecartHopper *entityminecarthopper) override;
	void displayGUIRepair(int_t x, int_t y, int_t z) override;
	void displayGUIBeacon(TileEntityBeacon *beacon) override;
	void displayGUIChest(IInventory *iinventory) override;
	void displayWorkbenchGUI(int_t i, int_t j, int_t k) override;
	void displayGUIFurnace(TileEntityFurnace *tileentityfurnace) override;
	void displayGUIDispenser(TileEntityDispenser *tileentitydispenser) override;
	void displayGUIEnchantment(int_t i, int_t j, int_t k) override;
	void displayGUIBrewingStand(TileEntityBrewingStand *tileentitybrewingstand) override;
	void displayGUIMerchant(IMerchant *merchant, MerchantRecipeList *recipes) override;
	void onCriticalHit(Entity *entity) override;
	void onEnchantmentCritical(Entity *entity) override;
	void onItemPickup(Entity *entity, int_t i) override;
	int_t getPlayerArmorValue() override;
	virtual void sendChatMessage(const std::string &s);
	bool isSneaking() override;
	void setHealth(int_t i) override;
	void respawnPlayer() override;
	void travelToTheEnd(int_t dimension) override;
	void handleItemUseFinish() override;
	void addChatMessage(const std::string &s) override;
	void addStat(StatBase *statbase, int_t i) override;
	void setXPStats(float progress, int_t total, int_t level);

private:
	bool isBlockTranslucent(int_t i, int_t j, int_t k);
	bool pushOutOfBlocks(double d, double d1, double d2) override;

public:
	MovementInput *movementInput;
	int_t sprintingTicksLeft;
	float renderArmYaw;
	float renderArmPitch;
	float prevRenderArmYaw;
	float prevRenderArmPitch;

protected:
	bool isClientWorld() const override;

	Minecraft *mc;
	int_t sprintToggleTimer;
};
