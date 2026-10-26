#pragma once
#include "Item.h"
class ItemCarrotOnAStick : public Item { public: explicit ItemCarrotOnAStick(int id) : Item(id) { setMaxStackSize(1); setMaxDamage(25); } };
