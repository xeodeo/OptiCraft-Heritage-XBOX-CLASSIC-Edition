#include "ContainerBeacon.h"
#include "TileEntityBeacon.h"
#include "InventoryPlayer.h"
#include "Slot.h"
ContainerBeacon::ContainerBeacon(InventoryPlayer *p, TileEntityBeacon *b) : beacon(b) { addSlot(new Slot(b,0,136,110)); for(int i=0;i<3;++i)for(int j=0;j<9;++j)addSlot(new Slot(p,j+i*9+9,8+j*18,140+i*18)); for(int j=0;j<9;++j)addSlot(new Slot(p,j,8+j*18,198)); }
bool ContainerBeacon::isUsableByPlayer(EntityPlayer *p) { return beacon != nullptr && beacon->canInteractWith(p); }
