#include "ItemWritableBook.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "NBTTagString.h"
#include "EntityPlayer.h"
ItemStack *ItemWritableBook::onItemRightClick(ItemStack*s,World*,EntityPlayer*p){if(p)p->displayGUIBook(s);return s;}
bool ItemWritableBook::validBookTagPages(NBTTagCompound*n){if(!n||!n->hasKey("pages"))return false;NBTTagList*l=dynamic_cast<NBTTagList*>(n->getTag("pages"));if(!l)return false;for(int i=0;i<l->tagCount();++i){auto*t=dynamic_cast<NBTTagString*>(l->tagAt(i));if(!t||t->stringValue.size()>256)return false;}return true;}
