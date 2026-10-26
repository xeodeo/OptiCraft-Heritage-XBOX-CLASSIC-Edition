#pragma once

class IInventory;
class World;

// net.minecraft.src.Hopper
class Hopper
{
public:
	virtual ~Hopper() = default;

	virtual World *getWorldObj() = 0;
	virtual double getXPos() = 0;
	virtual double getYPos() = 0;
	virtual double getZPos() = 0;
	virtual IInventory *getHopperInventory() = 0;
};
