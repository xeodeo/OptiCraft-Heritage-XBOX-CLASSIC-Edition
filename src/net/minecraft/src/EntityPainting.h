#pragma once

#include "EntityHanging.h"
#include "java/Type.h"

class World;
class EnumArt;
class NBTTagCompound;

// net.minecraft.src.EntityPainting
class EntityPainting : public EntityHanging
{
public:
	explicit EntityPainting(World *world);
	EntityPainting(World *world, int_t i, int_t j, int_t k, int_t l);
	EntityPainting(World *world, int_t i, int_t j, int_t k, int_t l, const jstring &s);

	static constexpr int_t CLASS_ID = 10;
	int_t getEntityClassID() const override { return CLASS_ID; }

	void writeEntityToNBT(NBTTagCompound *nbttagcompound) override;
	void readEntityFromNBT(NBTTagCompound *nbttagcompound) override;

	int func_82329_d() override;
	int func_82330_g() override;
	void dropItemStack() override;

public:
	const EnumArt *art = nullptr;
};
