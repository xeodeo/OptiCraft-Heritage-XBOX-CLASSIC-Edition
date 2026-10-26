#include "ContainerRepair.h"
#include "InventoryBasic.h"
#include "InventoryPlayer.h"
#include "Slot.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "World.h"
ContainerRepair::ContainerRepair(InventoryPlayer *p, World *, int, int, int) : repair(new InventoryBasic("container.repair", 3)), playerInventory(p)
{
    addSlot(new Slot(repair,0,27,47)); addSlot(new Slot(repair,1,76,47)); addSlot(new Slot(repair,2,134,47));
    for (int i=0;i<3;++i) for (int j=0;j<9;++j) addSlot(new Slot(p,j+i*9+9,8+j*18,84+i*18));
    for (int j=0;j<9;++j) addSlot(new Slot(p,j,8+j*18,142));
}
ContainerRepair::~ContainerRepair() { delete repair; }
bool ContainerRepair::isUsableByPlayer(EntityPlayer *p) { return p != nullptr && playerInventory != nullptr; }
void ContainerRepair::onCraftMatrixChanged(IInventory *inv) { (void)inv; }
ItemStack *ContainerRepair::getStackInSlot(int_t i) { return Container::getStackInSlot(i); }
