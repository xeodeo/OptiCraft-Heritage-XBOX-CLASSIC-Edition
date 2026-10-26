#include "BlockHopper.h"
#include "TileEntityHopper.h"
#include "World.h"
#include "IBlockAccess.h"
#include "Material.h"
#include "Facing.h"
#include "EntityPlayer.h"
#include "EntityItem.h"
#include "ItemStack.h"
#include "NBTTagCompound.h"
#include "Container.h"
#include "Texture152Atlas.h"
#include "StepSound.h"

BlockHopper::BlockHopper(int_t id)
	: BlockContainer(id, Material::iron)
{
	setHardness(3.0f);
	setResistance(8.0f);
	setStepSound(soundMetalFootstep);
	setBlockName("hopper");
	setRequiresSelfNotify();
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void BlockHopper::setBlockBoundsBasedOnState(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	(void)iblockaccess; (void)i; (void)j; (void)k;
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void BlockHopper::getCollidingBoundingBoxes(World *world, int_t i, int_t j, int_t k, AxisAlignedBB *axisalignedbb, std::vector<AxisAlignedBB *> &list)
{
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 0.625f, 1.0f);
	Block::getCollidingBoundingBoxes(world, i, j, k, axisalignedbb, list);
	const float f = 0.125f;
	setBlockBounds(0.0f, 0.0f, 0.0f, f, 1.0f, 1.0f);
	Block::getCollidingBoundingBoxes(world, i, j, k, axisalignedbb, list);
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, f);
	Block::getCollidingBoundingBoxes(world, i, j, k, axisalignedbb, list);
	setBlockBounds(1.0f - f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	Block::getCollidingBoundingBoxes(world, i, j, k, axisalignedbb, list);
	setBlockBounds(0.0f, 0.0f, 1.0f - f, 1.0f, 1.0f, 1.0f);
	Block::getCollidingBoundingBoxes(world, i, j, k, axisalignedbb, list);
	setBlockBounds(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

void BlockHopper::onBlockPlaced(World *world, int_t i, int_t j, int_t k, int_t l)
{
	int_t dir = FacingUtil::oppositeSide[l];
	if (dir == 1)
		dir = 0;
	world->setBlockMetadataWithNotify152(i, j, k, dir, 2);
}

TileEntity *BlockHopper::getBlockEntity()
{
	return new TileEntityHopper();
}

void BlockHopper::onBlockAdded(World *world, int_t i, int_t j, int_t k)
{
	BlockContainer::onBlockAdded(world, i, j, k);
	updateMetadata(world, i, j, k);
}

bool BlockHopper::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
	if (!world->multiplayerWorld)
	{
		TileEntityHopper *hopper = getHopperTile(world, i, j, k);
		if (hopper != nullptr)
		{
			entityplayer->displayGUIHopper(hopper);
		}
	}
	return true;
}

void BlockHopper::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	updateMetadata(world, i, j, k);
}

void BlockHopper::updateMetadata(World *world, int_t i, int_t j, int_t k)
{
	int_t meta = world->getBlockMetadata(i, j, k);
	int_t dir = getDirectionFromMetadata(meta);
	bool powered = world->isBlockIndirectlyGettingPowered(i, j, k);
	bool notPowered = getIsBlockNotPoweredFromMetadata(meta);
	if (powered == notPowered)
	{
		world->setBlockMetadataWithNotify152(i, j, k, dir | (powered ? 8 : 0), 4);
	}
}

void BlockHopper::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
	TileEntityHopper *hopper = getHopperTile(world, i, j, k);
	if (hopper != nullptr)
	{
		for (int_t slot = 0; slot < hopper->getSizeInventory(); ++slot)
		{
			ItemStack *stack = hopper->getStackInSlot(slot);
			if (stack != nullptr)
			{
				float rx = world->rand.nextFloat() * 0.8f + 0.1f;
				float ry = world->rand.nextFloat() * 0.8f + 0.1f;
				float rz = world->rand.nextFloat() * 0.8f + 0.1f;

				while (stack->stackSize > 0)
				{
					int_t toDrop = world->rand.nextInt(21) + 10;
					if (toDrop > stack->stackSize)
						toDrop = stack->stackSize;
					stack->stackSize -= toDrop;

					EntityItem *entityItem = new EntityItem(
						world,
						(double)((float)i + rx),
						(double)((float)j + ry),
						(double)((float)k + rz),
						new ItemStack(stack->itemID, toDrop, stack->getItemDamage())
					);
					if (stack->hasTagCompound())
					{
						entityItem->item->setTagCompound(dynamic_cast<NBTTagCompound *>(stack->getTagCompound()->copy()));
					}
					const float speed = 0.05f;
					entityItem->motionX = (double)((float)world->rand.nextGaussian() * speed);
					entityItem->motionY = (double)((float)world->rand.nextGaussian() * speed + 0.2f);
					entityItem->motionZ = (double)((float)world->rand.nextGaussian() * speed);
					world->entityJoinedWorld(entityItem);
				}
			}
		}
		world->func_96440_m(i, j, k, blockID);
	}
	world->removeBlockTileEntity(i, j, k);
	BlockContainer::onBlockRemoval(world, i, j, k);
}

int_t BlockHopper::getRenderType()
{
	return 38;
}

bool BlockHopper::renderAsNormalBlock()
{
	return false;
}

bool BlockHopper::isOpaqueCube()
{
	return false;
}

bool BlockHopper::shouldSideBeRendered(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
	(void)iblockaccess; (void)i; (void)j; (void)k; (void)l;
	return true;
}

int_t BlockHopper::getBlockTextureFromSideAndMetadata(int_t i, int_t j)
{
	(void)j;
	return i == 1 ? TEX152_BLOCK_ICON("hopper_top.png") : TEX152_BLOCK_ICON("hopper.png");
}

int_t BlockHopper::getBlockTextureFromSide(int_t i)
{
	return getBlockTextureFromSideAndMetadata(i, 0);
}

bool BlockHopper::hasComparatorInputOverride()
{
	return true;
}

int_t BlockHopper::getComparatorInputOverride(World *world, int_t i, int_t j, int_t k, int_t l)
{
	(void)l;
	return Container::calcRedstoneFromInventory(getHopperTile(world, i, j, k));
}

const char *BlockHopper::getItemIconName() const
{
	return "hopper";
}

int_t BlockHopper::getHopperIcon(const std::string &name)
{
	if (name == "hopper_inside") return TEX152_BLOCK_ICON("hopper_inside.png");
	return TEX152_BLOCK_ICON("hopper.png");
}

int_t BlockHopper::getDirectionFromMetadata(int_t metadata)
{
	return metadata & 7;
}

bool BlockHopper::getIsBlockNotPoweredFromMetadata(int_t metadata)
{
	return (metadata & 8) != 8;
}

TileEntityHopper *BlockHopper::getHopperTile(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k)
{
	return dynamic_cast<TileEntityHopper *>(iblockaccess->getBlockTileEntity(i, j, k));
}
