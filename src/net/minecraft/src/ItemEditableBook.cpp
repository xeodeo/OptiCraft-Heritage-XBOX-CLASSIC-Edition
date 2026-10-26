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
void ItemEditableBook::addInformation(ItemStack*s,std::vector<std::string>&out){if(s&&s->hasTagCompound()&&s->getTagCompound()->hasKey("author"))out.push_back("by "+s->getTagCompound()->getString("author"));}
std::string ItemEditableBook::title(NBTTagCompound*n){return n&&n->hasKey("title")?n->getString("title"):"";}
std::string ItemEditableBook::author(NBTTagCompound*n){return n&&n->hasKey("author")?n->getString("author"):"";}
int ItemEditableBook::pageCount(NBTTagCompound*n){return n?ItemWritableBook::pageCount(n):0;}
bool ItemEditableBook::isSigned(NBTTagCompound*n){return !author(n).empty()&&n&&n->hasKey("title");}
bool ItemEditableBook::hasPages(NBTTagCompound*n){return ItemWritableBook::hasPages(n);}
int ItemEditableBook::titleLength(NBTTagCompound*n){return (int)title(n).size();}
bool ItemEditableBook::canOpen(NBTTagCompound*n){return n==nullptr||validBookTagContents(n)||ItemWritableBook::validBookTagPages(n);}
