#include "BlockBeacon.h"

#include "Block.h"
#include "EntityPlayer.h"
#include "Material.h"
#include "TileEntityBeacon.h"
#include "World.h"
#include "EntityPlayerSP.h"
#include "Texture152Atlas.h"

BlockBeacon::BlockBeacon(int_t id)
	: BlockContainer(id, Material::glass)
{
	setHardness(3.0f);
	setLightValue(1.0f);
	blockIndexInTexture = Texture152::blockIcon("beacon.png");
}

TileEntity *BlockBeacon::createNewTileEntity(World *world)
{
	(void)world;
	return new TileEntityBeacon();
}

TileEntity *BlockBeacon::getBlockEntity()
{
	return new TileEntityBeacon();
}

bool BlockBeacon::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	EntityPlayerSP *player = dynamic_cast<EntityPlayerSP *>(entityplayer);
	if (player != nullptr) {
		TileEntity *tile = world->getBlockTileEntity(i, j, k);
		if (TileEntityBeacon *beacon = dynamic_cast<TileEntityBeacon *>(tile)) player->displayGUIBeacon(beacon);
	}
	return true;
}

int_t BlockBeacon::getBeaconIcon() const
{
	return Texture152::blockIcon("beacon.png");
}
