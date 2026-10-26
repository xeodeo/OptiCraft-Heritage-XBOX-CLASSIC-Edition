#include "SlotRepair.h"
#include "ContainerRepair.h"
SlotRepair::SlotRepair(ContainerRepair *c,IInventory*i,int n,int x,int y):Slot(i,n,x,y),container(c){}
void SlotRepair::onPickupFromSlot(ItemStack *stack){container->takeOutput(nullptr);Slot::onPickupFromSlot(stack);}
bool SlotRepair::isItemValid(ItemStack *) { return false; }
bool SlotRepair::isOutputSlot() const { return true; }
bool SlotRepair::hasResult() const { return container && container->hasRepairResult(); }
int SlotRepair::requiredLevels() const { return container ? container->maximumCost : 0; }
ContainerRepair *SlotRepair::getRepairContainer() const { return container; }
void SlotRepair::consumeResult(ItemStack *stack) { if (stack) onPickupFromSlot(stack); }
bool SlotRepair::canAccept(ItemStack *) const { return false; }
bool SlotRepair::isEmpty() const { return !hasResult(); }
bool SlotRepair::isReady() const { return hasResult() && requiredLevels() >= 0; }
int SlotRepair::cost() const { return requiredLevels(); }
void SlotRepair::pickup() { if (hasResult()) onPickupFromSlot(container->getResult()); }
ItemStack *SlotRepair::result() const { return container ? container->getResult() : nullptr; }
