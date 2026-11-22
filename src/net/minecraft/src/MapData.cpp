#include "MapData.h"
#include "java/Arithmetic.h"

#include <algorithm>
#include <cstdint>
#include "MapInfo.h"
#include "MapCoord.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "InventoryPlayer.h"
#include "NBTTagCompound.h"
#include "MathHelper.h"
#include "EntityItemFrame.h"
#include "World.h"
#include "WorldInfo.h"
#include <cmath>
#include <string>

MapData::MapData(const std::string &s) :
	MapDataBase(s),
	xCenter(0),
	zCenter(0),
	dimension(0),
	scale(0)
{
	for (int_t i = 0; i < 16384; i++)
		colors[i] = 0;
}

MapData::~MapData()
{
	clearVisible();

	for (MapInfo *info : playersArrayList)
		delete info;
	playersArrayList.clear();
	playersHashMap.clear();
}

void MapData::readFromNBT(NBTTagCompound *nbttagcompound)
{
	dimension = nbttagcompound->getByte("dimension");
	xCenter = nbttagcompound->getInteger("xCenter");
	zCenter = nbttagcompound->getInteger("zCenter");
	scale = nbttagcompound->getByte("scale");
	if (scale < 0) scale = 0;
	if (scale > 4) scale = 4;

	short_t width = nbttagcompound->getShort("width");
	short_t height = nbttagcompound->getShort("height");

	if (width == 128 && height == 128)
	{
		std::vector<byte_t> storedColors = nbttagcompound->getByteArray("colors");
		std::copy_n(storedColors.begin(), std::min<size_t>(storedColors.size(), 16384), colors);
	}
	else
	{
		std::vector<byte_t> storedColors = nbttagcompound->getByteArray("colors");
		for (int_t i = 0; i < 16384; i++)
			colors[i] = 0;

		int_t i = (128 - width) / 2;
		int_t j = (128 - height) / 2;
		for (int_t k = 0; k < height; k++)
		{
			int_t l = k + j;
			if (l < 0 || l >= 128) continue;
			for (int_t i1 = 0; i1 < width; i1++)
			{
				int_t j1 = i1 + i;
				if (j1 >= 0 && j1 < 128)
				{
					size_t sourceIndex = (size_t)i1 + (size_t)k * (size_t)width;
					if (sourceIndex < storedColors.size())
						colors[j1 + l * 128] = storedColors[sourceIndex];
				}
			}
		}
	}
}

void MapData::writeToNBT(NBTTagCompound *nbttagcompound)
{
	nbttagcompound->setByte("dimension", dimension);
	nbttagcompound->setInteger("xCenter", xCenter);
	nbttagcompound->setInteger("zCenter", zCenter);
	nbttagcompound->setByte("scale", scale);
	nbttagcompound->setShort("width", (short_t)128);
	nbttagcompound->setShort("height", (short_t)128);
	nbttagcompound->setByteArray("colors", std::vector<byte_t>(colors, colors + 16384));
}

void MapData::putVisible(const std::string &name, MapCoord *coord)
{
	for (auto &entry : playersVisibleOnMap)
	{
		if (entry.first == name)
		{
			delete entry.second;
			entry.second = coord;
			return;
		}
	}
	playersVisibleOnMap.emplace_back(name, coord);
}

void MapData::removeVisible(const std::string &name)
{
	for (auto it = playersVisibleOnMap.begin(); it != playersVisibleOnMap.end(); ++it)
	{
		if (it->first == name)
		{
			delete it->second;
			playersVisibleOnMap.erase(it);
			return;
		}
	}
}

void MapData::clearVisible()
{
	for (auto &entry : playersVisibleOnMap)
		delete entry.second;
	playersVisibleOnMap.clear();
}

void MapData::updateVisiblePlayers(EntityPlayer *entityplayer, ItemStack *itemstack)
{
	if (entityplayer == nullptr || itemstack == nullptr)
		return;

	if (playersHashMap.find(entityplayer) == playersHashMap.end())
	{
		MapInfo *mapinfo = new MapInfo(this, entityplayer);
		playersHashMap[entityplayer] = mapinfo;
		playersArrayList.push_back(mapinfo);
	}

	if (!entityplayer->inventory->hasItemStack(itemstack))
		removeVisible(entityplayer->username);

	for (size_t index = 0; index < playersArrayList.size(); )
	{
		MapInfo *mapinfo = playersArrayList[index];
		EntityPlayer *player = mapinfo->entityplayerObj;
		// Java only checks isDead. Here a respawned player is a new object and
		// the old one gets deleted, so a player no longer in the world's list
		// is dropped by pointer, before anything reads it.
		bool inWorld = false;
		if (entityplayer->worldObj != nullptr)
		{
			for (EntityPlayer *worldPlayer : entityplayer->worldObj->playerEntities)
			{
				if (worldPlayer == player)
				{
					inWorld = true;
					break;
				}
			}
		}
		if (player != nullptr && inWorld && !player->isDead &&
			(player->inventory->hasItemStack(itemstack) || itemstack->isOnItemFrame()))
		{
			if (!itemstack->isOnItemFrame() && player->dimension == dimension)
				func_82567_a(0, player->worldObj, player->username, player->posX, player->posZ, (double)player->rotationYaw);
			++index;
		}
		else
		{
			playersHashMap.erase(player);
			playersArrayList.erase(playersArrayList.begin() + index);
			delete mapinfo;
		}
	}

	if (itemstack->isOnItemFrame())
	{
		// Java 1.5.2: the green marker of the item frame holding this map.
		EntityItemFrame *frame = itemstack->getItemFrame();
		func_82567_a(1, entityplayer->worldObj, "frame-" + std::to_string(frame->entityId),
			(double)frame->xPosition, (double)frame->zPosition, (double)(frame->hangingDirection * 90));
	}
}

void MapData::func_82567_a(int_t iconType, World *world, const std::string &name, double x, double z, double rotation)
{
	const int_t blocksPerPixel = JavaArithmetic::intShl(1, scale);
	const float relX = (float)(x - (double)xCenter) / (float)blocksPerPixel;
	const float relZ = (float)(z - (double)zCenter) / (float)blocksPerPixel;
	byte_t iconX = JavaArithmetic::byteFromBits(static_cast<ubyte_t>(JavaArithmetic::doubleToInt((double)(relX * 2.0f) + 0.5)));
	byte_t iconZ = JavaArithmetic::byteFromBits(static_cast<ubyte_t>(JavaArithmetic::doubleToInt((double)(relZ * 2.0f) + 0.5)));
	const int_t edge = 63;
	byte_t iconRotation;
	if (relX >= (float)(-edge) && relZ >= (float)(-edge) && relX <= (float)edge && relZ <= (float)edge)
	{
		rotation += rotation < 0.0 ? -8.0 : 8.0;
		iconRotation = JavaArithmetic::byteFromBits(static_cast<ubyte_t>(JavaArithmetic::doubleToInt(rotation * 16.0 / 360.0)));
		if (dimension < 0 && world != nullptr && world->getWorldInfo() != nullptr)
		{
			// Java int arithmetic wraps at 32 bits.
			const uint32_t t = static_cast<uint32_t>(static_cast<int_t>(world->getWorldInfo()->getWorldTime() / 10LL));
			const int_t mixed = JavaArithmetic::intFromBits(t * t * 34187121u + t * 121u);
			iconRotation = static_cast<byte_t>((mixed >> 15) & 15);
		}
	}
	else
	{
		if (!(std::fabs(relX) < 320.0f) || !(std::fabs(relZ) < 320.0f))
		{
			removeVisible(name);
			return;
		}
		iconType = 6;
		iconRotation = 0;
		if (relX <= (float)(-edge))
			iconX = static_cast<byte_t>(JavaArithmetic::doubleToInt((double)(edge * 2) + 2.5));
		if (relZ <= (float)(-edge))
			iconZ = static_cast<byte_t>(JavaArithmetic::doubleToInt((double)(edge * 2) + 2.5));
		if (relX >= (float)edge)
			iconX = static_cast<byte_t>(edge * 2 + 1);
		if (relZ >= (float)edge)
			iconZ = static_cast<byte_t>(edge * 2 + 1);
	}

	putVisible(name, new MapCoord(this, static_cast<byte_t>(iconType), iconX, iconZ, iconRotation));
}

MapInfo *MapData::func_82568_a(EntityPlayer *entityplayer)
{
	auto it = playersHashMap.find(entityplayer);
	if (it != playersHashMap.end())
		return it->second;
	MapInfo *mapinfo = new MapInfo(this, entityplayer);
	playersHashMap[entityplayer] = mapinfo;
	playersArrayList.push_back(mapinfo);
	return mapinfo;
}

void MapData::setColumnDirty(int_t i, int_t j, int_t k)
{
	MapDataBase::markDirty();
	for (auto *mapinfo : playersArrayList)
	{
		if (mapinfo->minVisibleY[i] < 0 || mapinfo->minVisibleY[i] > j)
			mapinfo->minVisibleY[i] = j;
		if (mapinfo->maxVisibleY[i] < 0 || mapinfo->maxVisibleY[i] < k)
			mapinfo->maxVisibleY[i] = k;
	}
}

void MapData::updateMPMapData(const std::vector<byte_t> &data)
{
	if (data.empty())
		return;

	if (data[0] == 0 && data.size() >= 3)
	{
		int_t column = data[1] & 0xff;
		int_t firstRow = data[2] & 0xff;
		for (size_t offset = 3; offset < data.size(); offset++)
		{
			size_t row = (size_t)firstRow + offset - 3;
			if (column < 128 && row < 128)
				colors[row * 128 + (size_t)column] = data[offset];
		}
		markDirty();
	}
	else if (data[0] == 1)
	{
		clearVisible();
		for (size_t i = 0; i < (data.size() - 1) / 3; i++)
		{
			byte_t iconSize = static_cast<byte_t>(data[i * 3 + 1] >> 4);
			byte_t centerX = data[i * 3 + 2];
			byte_t centerZ = data[i * 3 + 3];
			byte_t rotation = static_cast<byte_t>(data[i * 3 + 1] & 15);
			playersVisibleOnMap.emplace_back("icon-" + std::to_string(i), new MapCoord(this, iconSize, centerX, centerZ, rotation));
		}
	}
	else if (data[0] == 2 && data.size() >= 2)
	{
		scale = data[1];
	}
}
