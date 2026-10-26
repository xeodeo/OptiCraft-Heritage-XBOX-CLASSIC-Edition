#pragma once
#include "Item.h"
class ItemEnchantedBook : public Item { public: explicit ItemEnchantedBook(int id) : Item(id) { setMaxStackSize(1); } };
