#include "ItemEnchantedBook.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "EnchantmentData.h"
#include "Enchantment.h"
#include "NBTTagCompound.h"
bool ItemEnchantedBook::hasEffect(ItemStack*){return true;}
bool ItemEnchantedBook::isItemTool(ItemStack*){return false;}
const EnumRarity& ItemEnchantedBook::getRarity(ItemStack*s){return func_92110_g(s)->tagCount()?EnumRarity::uncommon:EnumRarity::common;}
NBTTagList*ItemEnchantedBook::func_92110_g(ItemStack*s){if(s&&s->hasTagCompound()&&s->getTagCompound()->hasKey("StoredEnchantments"))return dynamic_cast<NBTTagList*>(s->getTagCompound()->getTag("StoredEnchantments"));return new NBTTagList();}
void ItemEnchantedBook::func_92115_a(ItemStack*s,EnchantmentData*d){if(!s||!d)return; if(!s->hasTagCompound())s->setTagCompound(new NBTTagCompound()); NBTTagList*l=func_92110_g(s); NBTTagCompound*n=new NBTTagCompound();n->setShort("id",(short)d->enchantmentobj->effectId);n->setShort("lvl",(short)d->enchantmentLevel);l->appendTag(n);s->getTagCompound()->setTag("StoredEnchantments",l);}
ItemStack*ItemEnchantedBook::func_92111_a(EnchantmentData*d){ItemStack*s=new ItemStack(this);func_92115_a(s,d);return s;}
void ItemEnchantedBook::addInformation(ItemStack*s,std::vector<std::string>&out){Item::addInformation(s,out);NBTTagList*l=func_92110_g(s);for(int i=0;i<l->tagCount();++i){auto*n=dynamic_cast<NBTTagCompound*>(l->tagAt(i));if(n)out.push_back("Enchantment "+std::to_string(n->getShort("id"))+" "+std::to_string(n->getShort("lvl")));}}
int ItemEnchantedBook::storedCount(ItemStack*s){return func_92110_g(s)->tagCount();}
int ItemEnchantedBook::storedId(ItemStack*s,int i){auto*l=func_92110_g(s);if(i<0||i>=l->tagCount())return -1;auto*n=dynamic_cast<NBTTagCompound*>(l->tagAt(i));return n?n->getShort("id"):-1;}
int ItemEnchantedBook::storedLevel(ItemStack*s,int i){auto*l=func_92110_g(s);if(i<0||i>=l->tagCount())return 0;auto*n=dynamic_cast<NBTTagCompound*>(l->tagAt(i));return n?n->getShort("lvl"):0;}
bool ItemEnchantedBook::hasStoredEnchantment(ItemStack*s,int id){for(int i=0;i<storedCount(s);++i)if(storedId(s,i)==id)return true;return false;}
int ItemEnchantedBook::enchantmentIdAt(ItemStack*s,int i){return storedId(s,i);}
int ItemEnchantedBook::enchantmentLevelAt(ItemStack*s,int i){return storedLevel(s,i);}
bool ItemEnchantedBook::mergeEnchantment(ItemStack*s,int id,int level){if(!s)return false;auto*l=func_92110_g(s);for(int i=0;i<l->tagCount();++i){auto*n=dynamic_cast<NBTTagCompound*>(l->tagAt(i));if(n&&n->getShort("id")==id){if(n->getShort("lvl")<level)n->setShort("lvl",(short)level);return true;}}return false;}
bool ItemEnchantedBook::validStoredTag(ItemStack*s){return s!=nullptr&&s->hasTagCompound()&&s->getTagCompound()->hasKey("StoredEnchantments");}
void ItemEnchantedBook::clearStoredEnchantments(ItemStack*s){if(validStoredTag(s))s->setTagCompound(new NBTTagCompound());}
ItemStack*ItemEnchantedBook::copyWithEnchantment(ItemStack*s,int id,int level){ItemStack*r=s?s->copy():new ItemStack(this);if(!mergeEnchantment(r,id,level)){NBTTagCompound*n=new NBTTagCompound();n->setShort("id",(short)id);n->setShort("lvl",(short)level);if(!r->hasTagCompound())r->setTagCompound(new NBTTagCompound());auto*l=func_92110_g(r);l->appendTag(n);r->getTagCompound()->setTag("StoredEnchantments",l);}return r;}
bool ItemEnchantedBook::canApplyToItem(ItemStack*s,int id){return s!=nullptr&&id>=0&&id<256&&Enchantment::enchantmentsList[id]!=nullptr;}
int ItemEnchantedBook::clampLevel(int level)const{return level<1?1:(level>5?5:level);}
void ItemEnchantedBook::setStoredLevel(ItemStack*s,int id,int level){if(!s)return;mergeEnchantment(s,id,clampLevel(level));}
std::string ItemEnchantedBook::storedDescription(ItemStack*s){std::string out;for(int i=0;i<storedCount(s);++i){if(!out.empty())out+="; ";out+="Enchantment "+std::to_string(storedId(s,i))+" "+std::to_string(storedLevel(s,i));}return out;}
