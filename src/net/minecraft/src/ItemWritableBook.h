#pragma once
#include "Item.h"
class ItemWritableBook : public Item { public: explicit ItemWritableBook(int id) : Item(id) { setMaxStackSize(1); } };
