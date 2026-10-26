#include "ContainerRepair.h"
#include "InventoryPlayer.h"
#include "Slot.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "Item.h"
#include "World.h"
#include "Block.h"
#include "SlotRepair.h"
ContainerRepair::ContainerRepair(InventoryPlayer *p, World *w, int px, int py, int pz, EntityPlayer *ep) : input(new InventoryBasic("Repair",2)), output(new InventoryCraftResult()), playerInventory(p), world(w), owner(ep), x(px), y(py), z(pz) {
    input->addInventoryChangeCallback(this); addSlot(new Slot(input,0,27,47)); addSlot(new Slot(input,1,76,47)); addSlot(new SlotRepair(this,output,0,134,47));
    for(int i=0;i<3;++i) for(int j=0;j<9;++j) addSlot(new Slot(p,j+i*9+9,8+j*18,84+i*18)); for(int j=0;j<9;++j) addSlot(new Slot(p,j,8+j*18,142));
}
ContainerRepair::~ContainerRepair(){delete input;delete output;}
bool ContainerRepair::isUsableByPlayer(EntityPlayer *p){return world&&world->getBlockId(x,y,z)==Block::anvil->blockID&&p&&p->getDistanceSq(x+.5,y+.5,z+.5)<=64.0;}
void ContainerRepair::onCraftMatrixChanged(IInventory *i){if(i==input)updateRepairOutput();}
void ContainerRepair::onInventoryChanged(IInventory *i){if(i==input)updateRepairOutput();}
void ContainerRepair::updateRepairOutput(){ItemStack*l=input->getStackInSlot(0),*r=input->getStackInSlot(1);maximumCost=0;if(!l){output->setInventorySlotContents(0,nullptr);return;}ItemStack*o=l->copy();if(r&&r->itemID==l->itemID&&l->isItemStackDamageable()){int a=l->getMaxDamage()-l->getItemDamageForDisplay(),b=r->getMaxDamage()-r->getItemDamageForDisplay();int d=l->getItemDamageForDisplay()-(a+b+l->getMaxDamage()*12/100-l->getMaxDamage());if(d<0)d=0;if(d<l->getItemDamageForDisplay()){o->setItemDamage(d);maximumCost=1+l->getItemDamageForDisplay()/100;}}if(maximumCost==0&&!repairedItemName.empty())maximumCost=7;if(!maximumCost){delete o;output->setInventorySlotContents(0,nullptr);return;}output->setInventorySlotContents(0,o);}
void ContainerRepair::takeOutput(EntityPlayer*p){EntityPlayer*u=p?p:owner;if(!u||!output->getStackInSlot(0)||(!u->capabilities.isCreativeMode&&u->experienceLevel<maximumCost))return;if(!u->capabilities.isCreativeMode)u->removeExperience(maximumCost);input->decrStackSize(0,1);if(input->getStackInSlot(1))input->decrStackSize(1,1);output->setInventorySlotContents(0,nullptr);if(world&&!world->multiplayerWorld&&world->getBlockMetadata(x,y,z)<2)world->setBlockMetadataWithNotify152(x,y,z,world->getBlockMetadata(x,y,z)+1,2);}
void ContainerRepair::updateItemName(const std::string&n){repairedItemName=n;updateRepairOutput();}
ItemStack*ContainerRepair::getStackInSlot(int_t i){return Container::getStackInSlot(i);}
IInventory *ContainerRepair::getResultInventory() const { return output; }
ItemStack *ContainerRepair::getInput(int slot) const { return (slot >= 0 && slot < 2) ? input->getStackInSlot(slot) : nullptr; }
ItemStack *ContainerRepair::getResult() const { return output->getStackInSlot(0); }
bool ContainerRepair::hasRepairInputs() const { return getInput(0) != nullptr; }
bool ContainerRepair::hasRepairResult() const { return getResult() != nullptr; }
bool ContainerRepair::canTakeOutput(EntityPlayer *player) const {
    EntityPlayer *actual = player ? player : owner;
    return actual && hasRepairResult() && (actual->capabilities.isCreativeMode || actual->experienceLevel >= maximumCost);
}
bool ContainerRepair::sameRepairItem(const ItemStack *left, const ItemStack *right) const {
    return left && right && left->itemID == right->itemID && const_cast<ItemStack *>(left)->isItemStackDamageable();
}
int ContainerRepair::getRemainingDurability(const ItemStack *stack) const {
    ItemStack *mutableStack = const_cast<ItemStack *>(stack);
    return mutableStack ? mutableStack->getMaxDamage() - mutableStack->getItemDamageForDisplay() : 0;
}
int ContainerRepair::getDurabilityCost(const ItemStack *stack) const {
    ItemStack *mutableStack = const_cast<ItemStack *>(stack);
    return mutableStack ? 1 + mutableStack->getItemDamageForDisplay() / 100 : 0;
}
int ContainerRepair::calculateRepairCost(const ItemStack *left, const ItemStack *right) const {
    if (!sameRepairItem(left, right)) return 0;
    ItemStack *mutableLeft = const_cast<ItemStack *>(left);
    int max = mutableLeft->getMaxDamage();
    int repaired = mutableLeft->getItemDamageForDisplay() - (getRemainingDurability(left) + getRemainingDurability(right) + max * 12 / 100 - max);
    return repaired < mutableLeft->getItemDamageForDisplay() ? getDurabilityCost(left) : 0;
}
int ContainerRepair::calculateNameCost() const { return repairedItemName.empty() ? 0 : 7; }
ItemStack *ContainerRepair::buildRepairResult() const {
    ItemStack *left = getInput(0);
    if (!left) return nullptr;
    ItemStack *result = left->copy();
    ItemStack *right = getInput(1);
    if (sameRepairItem(left, right)) {
        ItemStack *mutableLeft = const_cast<ItemStack *>(left);
        int max = mutableLeft->getMaxDamage();
        int damage = mutableLeft->getItemDamageForDisplay() - (getRemainingDurability(left) + getRemainingDurability(right) + max * 12 / 100 - max);
        if (damage < 0) damage = 0;
        if (damage < left->getItemDamageForDisplay()) result->setItemDamage(damage);
    }
    return result;
}
void ContainerRepair::clearResult() { output->setInventorySlotContents(0, nullptr); }
void ContainerRepair::consumeInput(int slot) { if (slot >= 0 && slot < 2 && getInput(slot)) input->decrStackSize(slot, 1); }
void ContainerRepair::damageAnvil() {
    if (world && !world->multiplayerWorld && world->getBlockMetadata(x, y, z) < 2)
        world->setBlockMetadataWithNotify152(x, y, z, world->getBlockMetadata(x, y, z) + 1, 2);
}
bool ContainerRepair::isAnvilAtPosition() const { return world && world->getBlockId(x, y, z) == Block::anvil->blockID; }
bool ContainerRepair::isPlayerInRange(EntityPlayer *player) const { return player && player->getDistanceSq(x + .5, y + .5, z + .5) <= 64.0; }
void ContainerRepair::refreshAfterNameChange() { updateRepairOutput(); }
const std::string &ContainerRepair::getItemName() const { return repairedItemName; }
int ContainerRepair::getRepairX() const { return x; }
int ContainerRepair::getRepairY() const { return y; }
int ContainerRepair::getRepairZ() const { return z; }
World *ContainerRepair::getRepairWorld() const { return world; }
EntityPlayer *ContainerRepair::getOwner() const { return owner; }
bool ContainerRepair::hasNameChange() const { return !repairedItemName.empty(); }
bool ContainerRepair::nameMatchesInput() const { return hasNameChange() && getInput(0) != nullptr; }
void ContainerRepair::setMaximumCost(int cost) { maximumCost = cost < 0 ? 0 : cost; }
int ContainerRepair::getMaximumCost() const { return maximumCost; }
int ContainerRepair::getInputCount() const { return (getInput(0) ? 1 : 0) + (getInput(1) ? 1 : 0); }
bool ContainerRepair::inputSlotEmpty(int slot) const { return getInput(slot) == nullptr; }
bool ContainerRepair::outputSlotEmpty() const { return getResult() == nullptr; }
void ContainerRepair::consumeAllInputs() { consumeInput(0); consumeInput(1); }
void ContainerRepair::resetRepairState() { maximumCost = 0; clearResult(); }
bool ContainerRepair::validRepairPair() const { return sameRepairItem(getInput(0), getInput(1)); }
int ContainerRepair::repairedDamage() const {
    ItemStack *left = getInput(0), *right = getInput(1); if (!sameRepairItem(left, right)) return left ? const_cast<ItemStack *>(left)->getItemDamageForDisplay() : 0;
    int max = const_cast<ItemStack *>(left)->getMaxDamage(); int damage = const_cast<ItemStack *>(left)->getItemDamageForDisplay() - (getRemainingDurability(left) + getRemainingDurability(right) + max * 12 / 100 - max); return damage < 0 ? 0 : damage;
}
void ContainerRepair::publishResult(ItemStack *result) { output->setInventorySlotContents(0, result); }
void ContainerRepair::applyRepairDamage(ItemStack *result, int damage) const { if (result && result->isItemStackDamageable()) result->setItemDamage(damage); }
bool ContainerRepair::playerCanPay(EntityPlayer *player) const { return player && (player->capabilities.isCreativeMode || player->experienceLevel >= maximumCost); }
void ContainerRepair::chargePlayer(EntityPlayer *player) { if (player && !player->capabilities.isCreativeMode) player->removeExperience(maximumCost); }
void ContainerRepair::finishTransaction(EntityPlayer *player) { if (!playerCanPay(player) || !hasRepairResult()) return; chargePlayer(player); consumeAllInputs(); clearResult(); damageAnvil(); }
bool ContainerRepair::shouldDamageAnvil() const { return world && !world->multiplayerWorld && anvilMetadata() < 2; }
int ContainerRepair::anvilMetadata() const { return world ? world->getBlockMetadata(x, y, z) : 0; }
void ContainerRepair::notifyInputChanged() { updateRepairOutput(); }
bool ContainerRepair::isValidInputSlot(int slot) const { return slot == 0 || slot == 1; }
bool ContainerRepair::isValidOutputSlot(int slot) const { return slot == 0; }
int ContainerRepair::countRepairableInputs() const { return validRepairPair() ? 2 : (getInput(0) ? 1 : 0); }
int ContainerRepair::countMissingInputs() const { return 2 - countRepairableInputs(); }
bool ContainerRepair::needsExperience() const { return maximumCost > 0; }
bool ContainerRepair::needsAnvilDamage() const { return shouldDamageAnvil(); }
void ContainerRepair::setName(const std::string &name) { repairedItemName = name; refreshAfterNameChange(); }
void ContainerRepair::clearName() { repairedItemName.clear(); refreshAfterNameChange(); }
bool ContainerRepair::hasOwner() const { return owner != nullptr; }
bool ContainerRepair::hasWorld() const { return world != nullptr; }
bool ContainerRepair::ownsPlayer(EntityPlayer *player) const { return player && player == owner; }
void ContainerRepair::copyInputToResult() { ItemStack *result = buildRepairResult(); publishResult(result); }
void ContainerRepair::rebuildCost() { maximumCost = calculateRepairCost(getInput(0), getInput(1)); if (!maximumCost) maximumCost = calculateNameCost(); }
void ContainerRepair::rebuildResult() { clearResult(); if (hasRepairInputs()) copyInputToResult(); }
void ContainerRepair::commitResult() { if (hasRepairResult()) finishTransaction(owner); }
void ContainerRepair::abortResult() { resetRepairState(); }
bool ContainerRepair::transactionReady(EntityPlayer *player) const { return hasRepairResult() && playerCanPay(player); }
