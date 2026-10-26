#include "ItemEnchantedBook.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "EnchantmentData.h"
#include "Enchantment.h"
bool ItemEnchantedBook::hasEffect(ItemStack*){return true;}
bool ItemEnchantedBook::isItemTool(ItemStack*){return false;}
const EnumRarity& ItemEnchantedBook::getRarity(ItemStack*s){return func_92110_g(s)->tagCount()?EnumRarity::uncommon:EnumRarity::common;}
NBTTagList*ItemEnchantedBook::func_92110_g(ItemStack*s){if(s&&s->hasTagCompound()&&s->getTagCompound()->hasKey("StoredEnchantments"))return dynamic_cast<NBTTagList*>(s->getTagCompound()->getTag("StoredEnchantments"));return new NBTTagList();}
void ItemEnchantedBook::func_92115_a(ItemStack*s,EnchantmentData*d){if(!s||!d)return; if(!s->hasTagCompound())s->setTagCompound(new NBTTagCompound()); NBTTagList*l=func_92110_g(s); NBTTagCompound*n=new NBTTagCompound();n->setShort("id",(short)d->enchantmentobj->effectId);n->setShort("lvl",(short)d->enchantmentLevel);l->appendTag(n);s->getTagCompound()->setTag("StoredEnchantments",l);}
ItemStack*ItemEnchantedBook::func_92111_a(EnchantmentData*d){ItemStack*s=new ItemStack(this);func_92115_a(s,d);return s;}
