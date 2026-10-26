#pragma once
#include "Item.h"
class ItemEditableBook : public Item { public: explicit ItemEditableBook(int id) : Item(id) { setMaxStackSize(1); } };
