#include "ContainerBeacon.h"
#include "TileEntityBeacon.h"
#include "InventoryPlayer.h"
#include "Slot.h"
#include "Item.h"
#include "ItemStack.h"
namespace { class SlotBeacon final : public Slot { public: SlotBeacon(IInventory*i,int n,int x,int y):Slot(i,n,x,y){} bool isItemValid(ItemStack*s) override { return s && (s->itemID==Item::ingotIron->shiftedIndex||s->itemID==Item::ingotGold->shiftedIndex||s->itemID==Item::emerald->shiftedIndex||s->itemID==Item::diamond->shiftedIndex); } }; }
ContainerBeacon::ContainerBeacon(InventoryPlayer *p, TileEntityBeacon *b) : beacon(b) { addSlot(new SlotBeacon(b,0,136,110)); for(int i=0;i<3;++i)for(int j=0;j<9;++j)addSlot(new Slot(p,j+i*9+9,8+j*18,140+i*18)); for(int j=0;j<9;++j)addSlot(new Slot(p,j,8+j*18,198)); }
bool ContainerBeacon::isUsableByPlayer(EntityPlayer *p) { return beacon != nullptr && beacon->canInteractWith(p); }
bool ContainerBeacon::enchantItem(EntityPlayer *p,int_t id){if(!beacon||!p||id<0)return false;int level=beacon->getLevels();if(id/16>level)return false;if(id%16==0)beacon->setPrimaryEffect(id);else beacon->setSecondaryEffect(id);return true;}
TileEntityBeacon *ContainerBeacon::getBeacon() const { return beacon; }
int ContainerBeacon::getBeaconLevel() const { return beacon ? beacon->getLevels() : 0; }
int ContainerBeacon::getPrimaryEffect() const { return beacon ? beacon->getPrimaryEffect() : 0; }
int ContainerBeacon::getSecondaryEffect() const { return beacon ? beacon->getSecondaryEffect() : 0; }
bool ContainerBeacon::isPayment(ItemStack *stack) const { return stack && (stack->itemID == Item::ingotIron->shiftedIndex || stack->itemID == Item::ingotGold->shiftedIndex || stack->itemID == Item::emerald->shiftedIndex || stack->itemID == Item::diamond->shiftedIndex); }
bool ContainerBeacon::canSelectEffect(int id) const { return id >= 0 && id / 16 <= getBeaconLevel(); }
bool ContainerBeacon::selectPrimaryEffect(int id) { if (!beacon || !canSelectEffect(id)) return false; beacon->setPrimaryEffect(id); return true; }
bool ContainerBeacon::selectSecondaryEffect(int id) { if (!beacon || !canSelectEffect(id)) return false; beacon->setSecondaryEffect(id); return true; }
ItemStack *ContainerBeacon::getPayment() const { return beacon ? beacon->getStackInSlot(0) : nullptr; }
bool ContainerBeacon::hasPayment() const { return isPayment(getPayment()); }
void ContainerBeacon::clearPayment() { if (beacon) beacon->setInventorySlotContents(0, nullptr); }
bool ContainerBeacon::hasBeacon() const { return beacon != nullptr; }
bool ContainerBeacon::paymentMatches(ItemStack *stack) const { return isPayment(stack); }
bool ContainerBeacon::validPrimary(int id) const { return id >= 0 && id % 16 == 0 && canSelectEffect(id); }
bool ContainerBeacon::validSecondary(int id) const { return id >= 0 && id % 16 != 0 && canSelectEffect(id); }
int ContainerBeacon::effectTier(int id) const { return id < 0 ? -1 : id / 16; }
int ContainerBeacon::effectId(int id) const { return id < 0 ? -1 : id % 16; }
void ContainerBeacon::resetEffects() { if (beacon) { beacon->setPrimaryEffect(0); beacon->setSecondaryEffect(0); } }
bool ContainerBeacon::isInRange(EntityPlayer *p) const { return beacon && beacon->canInteractWith(p); }
bool ContainerBeacon::isOpen() const { return hasBeacon(); }
bool ContainerBeacon::hasPrimaryEffect() const { return getPrimaryEffect() != 0; }
bool ContainerBeacon::hasSecondaryEffect() const { return getSecondaryEffect() != 0; }
bool ContainerBeacon::canPay() const { return hasPayment(); }
int ContainerBeacon::paymentItemId() const { ItemStack *stack = getPayment(); return stack ? stack->itemID : 0; }
