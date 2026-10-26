#include "ItemEditableBook.h"
#include "ItemWritableBook.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagString.h"
#include "EntityPlayer.h"
bool ItemEditableBook::validBookTagContents(NBTTagCompound*n){return ItemWritableBook::validBookTagPages(n)&&n->hasKey("title")&&n->getString("title").size()<=16&&n->hasKey("author");}
ItemStack *ItemEditableBook::onItemRightClick(ItemStack*s,World*,EntityPlayer*p){if(p)p->displayGUIBook(s);return s;}
bool ItemEditableBook::hasEffect(ItemStack*){return true;}
std::string ItemEditableBook::getItemDisplayName(ItemStack*s){if(s&&s->hasTagCompound()&&s->getTagCompound()->hasKey("title"))return s->getTagCompound()->getString("title");return Item::getItemDisplayName(s);}
