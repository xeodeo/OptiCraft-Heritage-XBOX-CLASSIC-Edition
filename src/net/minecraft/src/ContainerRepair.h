#pragma once
#include "Container.h"
#include "IInventory.h"
#include "IInvBasic.h"
#include "InventoryBasic.h"
#include "InventoryCraftResult.h"
class InventoryPlayer;
class World;
class ContainerRepair : public Container, public IInvBasic {
public:
    ContainerRepair(InventoryPlayer *player, World *world, int x, int y, int z, EntityPlayer *owner);
    ~ContainerRepair() override;
    bool isUsableByPlayer(EntityPlayer *player) override;
    void onCraftMatrixChanged(IInventory *inventory) override;
    void onInventoryChanged(IInventory *inventory) override;
    void updateRepairOutput();
    void takeOutput(EntityPlayer *player);
    void updateItemName(const std::string &name);
    ItemStack *getStackInSlot(int_t index) override;
    IInventory *getRepairInventory() const { return input; }
    IInventory *getResultInventory() const;
    ItemStack *getInput(int slot) const;
    ItemStack *getResult() const;
    bool hasRepairInputs() const;
    bool hasRepairResult() const;
    bool canTakeOutput(EntityPlayer *player) const;
    bool sameRepairItem(const ItemStack *left, const ItemStack *right) const;
    int getRemainingDurability(const ItemStack *stack) const;
    int getDurabilityCost(const ItemStack *stack) const;
    int calculateRepairCost(const ItemStack *left, const ItemStack *right) const;
    int calculateNameCost() const;
    ItemStack *buildRepairResult() const;
    void clearResult();
    void consumeInput(int slot);
    void damageAnvil();
    bool isAnvilAtPosition() const;
    bool isPlayerInRange(EntityPlayer *player) const;
    void refreshAfterNameChange();
    const std::string &getItemName() const;
    int getRepairX() const;
    int getRepairY() const;
    int getRepairZ() const;
    World *getRepairWorld() const;
    EntityPlayer *getOwner() const;
    bool hasNameChange() const;
    bool nameMatchesInput() const;
    void setMaximumCost(int cost);
    int getMaximumCost() const;
    int getInputCount() const;
    bool inputSlotEmpty(int slot) const;
    bool outputSlotEmpty() const;
    void consumeAllInputs();
    void resetRepairState();
    bool validRepairPair() const;
    int repairedDamage() const;
    void publishResult(ItemStack *result);
    void applyRepairDamage(ItemStack *result, int damage) const;
    bool playerCanPay(EntityPlayer *player) const;
    void chargePlayer(EntityPlayer *player);
    void finishTransaction(EntityPlayer *player);
    bool shouldDamageAnvil() const;
    int anvilMetadata() const;
    void notifyInputChanged();
    bool isValidInputSlot(int slot) const;
    bool isValidOutputSlot(int slot) const;
    int countRepairableInputs() const;
    int countMissingInputs() const;
    bool needsExperience() const;
    bool needsAnvilDamage() const;
    void setName(const std::string &name);
    void clearName();
    bool hasOwner() const;
    bool hasWorld() const;
    bool ownsPlayer(EntityPlayer *player) const;
    void copyInputToResult();
    void rebuildCost();
    void rebuildResult();
    void commitResult();
    void abortResult();
    bool transactionReady(EntityPlayer *player) const;
    int maximumCost = 0;
private:
    InventoryBasic *input;
    InventoryCraftResult *output;
    InventoryPlayer *playerInventory;
    World *world;
    EntityPlayer *owner;
    int x, y, z;
    std::string repairedItemName;
};
