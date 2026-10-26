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
