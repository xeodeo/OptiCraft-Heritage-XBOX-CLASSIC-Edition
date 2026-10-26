#include "SlotRepair.h"
#include "ContainerRepair.h"
SlotRepair::SlotRepair(ContainerRepair *c,IInventory*i,int n,int x,int y):Slot(i,n,x,y),container(c){}
void SlotRepair::onPickupFromSlot(ItemStack *stack){container->takeOutput(nullptr);Slot::onPickupFromSlot(stack);}
