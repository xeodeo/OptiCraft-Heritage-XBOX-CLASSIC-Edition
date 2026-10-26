#pragma once
#include "Container.h"
class InventoryPlayer;
class TileEntityBeacon;
class ContainerBeacon : public Container { public: ContainerBeacon(InventoryPlayer *p, TileEntityBeacon *b); bool isUsableByPlayer(EntityPlayer *p) override; private: TileEntityBeacon *beacon; };
