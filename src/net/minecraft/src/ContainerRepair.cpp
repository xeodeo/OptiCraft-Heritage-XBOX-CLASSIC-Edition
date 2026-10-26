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
