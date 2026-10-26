#include "ItemWritableBook.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "NBTTagString.h"
#include "EntityPlayer.h"
ItemStack *ItemWritableBook::onItemRightClick(ItemStack*s,World*,EntityPlayer*p){if(p)p->displayGUIBook(s);return s;}
bool ItemWritableBook::validBookTagPages(NBTTagCompound*n){if(!n||!n->hasKey("pages"))return false;NBTTagList*l=dynamic_cast<NBTTagList*>(n->getTag("pages"));if(!l)return false;for(int i=0;i<l->tagCount();++i){auto*t=dynamic_cast<NBTTagString*>(l->tagAt(i));if(!t||t->stringValue.size()>256)return false;}return true;}
int ItemWritableBook::pageCount(NBTTagCompound*n){if(!validBookTagPages(n))return 0;return dynamic_cast<NBTTagList*>(n->getTag("pages"))->tagCount();}
std::string ItemWritableBook::sanitizePage(const std::string&p){return p.size()>256?p.substr(0,256):p;}
bool ItemWritableBook::hasPages(NBTTagCompound*n){return pageCount(n)>0;}
std::string ItemWritableBook::pageAt(NBTTagCompound*n,int i){if(!validBookTagPages(n))return "";auto*l=dynamic_cast<NBTTagList*>(n->getTag("pages"));if(i<0||i>=l->tagCount())return "";auto*t=dynamic_cast<NBTTagString*>(l->tagAt(i));return t?t->stringValue:"";}
void ItemWritableBook::appendPage(NBTTagCompound*n,const std::string&p){if(!n)return;if(!n->hasKey("pages"))n->setTag("pages",new NBTTagList());auto*l=dynamic_cast<NBTTagList*>(n->getTag("pages"));l->appendTag(new NBTTagString(sanitizePage(p)));}
