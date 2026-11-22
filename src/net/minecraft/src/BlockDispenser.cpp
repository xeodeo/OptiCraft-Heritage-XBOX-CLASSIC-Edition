#include "BlockDispenser.h"
#include "AxisAlignedBB.h"
#include "Block.h"
#include "BlockFire.h"
#include "BlockPistonBase.h"
#include "BlockRail.h"
#include "Container.h"
#include "EntityArrow.h"
#include "EntityBoat.h"
#include "EntityEgg.h"
#include "EntityExpBottle.h"
#include "EntityFireworkRocket.h"
#include "EntityItem.h"
#include "EntityLiving.h"
#include "EntityMinecart.h"
#include "EntityPlayer.h"
#include "EntityPotion.h"
#include "EntitySmallFireball.h"
#include "EntitySnowball.h"
#include "EntityTNTPrimed.h"
#include "Facing.h"
#include "IBlockAccess.h"
#include "InventoryPlayer.h"
#include "Item.h"
#include "ItemArmor.h"
#include "ItemBucket.h"
#include "ItemDye.h"
#include "ItemMinecart.h"
#include "ItemMonsterPlacer.h"
#include "ItemPotion.h"
#include "ItemStack.h"
#include "Material.h"
#include "MathHelper.h"
#include "NBTTagCompound.h"
#include "Texture152Atlas.h"
#include "TileEntity.h"
#include "TileEntityDispenser.h"
#include "World.h"

namespace
{
int_t encodeDispenserSmokeDir(int_t dir)
{
    if (dir == 0)
        return 9;
    if (dir == 1)
        return 10;
    const int_t dx = FacingUtil::offsetsXForSide[dir];
    const int_t dz = FacingUtil::offsetsZForSide[dir];
    return dx + 1 + (dz + 1) * 3;
}

void dropItemFromDispenser(World *world, int_t i, int_t j, int_t k, int_t dir, ItemStack *droppedStack, Random &random)
{
    const int_t dx = FacingUtil::offsetsXForSide[dir];
    const int_t dy = FacingUtil::offsetsYForSide[dir];
    const int_t dz = FacingUtil::offsetsZForSide[dir];

    double x = (double)i + (double)dx * 0.7 + 0.5;
    double y = (double)j + (double)dy * 0.7 + 0.5;
    double z = (double)k + (double)dz * 0.7 + 0.5;
    if (dir == 0 || dir == 1)
        y -= 0.125;
    else
        y -= 0.15625;

    EntityItem *entity = new EntityItem(world, x, y, z, droppedStack);
    const double speed = random.nextDouble() * 0.1 + 0.2;
    entity->motionX = (double)dx * speed;
    entity->motionY = (dy == 0) ? 0.20000000298023224 : ((double)dy * speed);
    entity->motionZ = (double)dz * speed;
    entity->motionX += random.nextGaussian() * 0.007499999832361937 * 6.0;
    entity->motionY += random.nextGaussian() * 0.007499999832361937 * 6.0;
    entity->motionZ += random.nextGaussian() * 0.007499999832361937 * 6.0;
    if (!world->entityJoinedWorld(entity))
    {
        delete entity;
    }
    world->playAuxSFX(1000, i, j, k, 0);
    world->playAuxSFX(2000, i, j, k, encodeDispenserSmokeDir(dir));
}
} // namespace

BlockDispenser::BlockDispenser(int_t i) : BlockContainer(i, Material::rock)
{
    blockIndexInTexture = 45;
}

int_t BlockDispenser::tickRate() { return 4; }

int_t BlockDispenser::idDropped(int_t i, Random &random)
{
    (void)i;
    (void)random;
    return Block::dispenser->blockID;
}

void BlockDispenser::onBlockAdded(World *world, int_t i, int_t j, int_t k)
{
    BlockContainer::onBlockAdded(world, i, j, k);
    setDispenserDefaultDirection(world, i, j, k);
}

void BlockDispenser::setDispenserDefaultDirection(World *world, int_t i, int_t j, int_t k)
{
    if (world->multiplayerWorld) return;
    int_t l  = world->getBlockId(i, j, k - 1);
    int_t i1 = world->getBlockId(i, j, k + 1);
    int_t j1 = world->getBlockId(i - 1, j, k);
    int_t k1 = world->getBlockId(i + 1, j, k);
    byte_t byte0 = 3;
    if (Block::opaqueCubeLookup[l]  && !Block::opaqueCubeLookup[i1]) byte0 = 3;
    if (Block::opaqueCubeLookup[i1] && !Block::opaqueCubeLookup[l])  byte0 = 2;
    if (Block::opaqueCubeLookup[j1] && !Block::opaqueCubeLookup[k1]) byte0 = 5;
    if (Block::opaqueCubeLookup[k1] && !Block::opaqueCubeLookup[j1]) byte0 = 4;
    world->setBlockMetadataWithNotify152(i, j, k, byte0, 2);
}

int_t BlockDispenser::getBlockTexture(IBlockAccess *iblockaccess, int_t i, int_t j, int_t k, int_t l)
{
    const int_t dir = iblockaccess->getBlockMetadata(i, j, k) & 7;
    if (l == dir)
    {
        return (dir == 0 || dir == 1)
            ? TEX152_BLOCK_ICON("dispenser_front_vertical.png")
            : TEX152_BLOCK_ICON("dispenser_front.png");
    }
    if (dir == 0 || dir == 1)
    {
        return 62;
    }
    return (l == 0 || l == 1) ? 62 : 45;
}

int_t BlockDispenser::getBlockTextureFromSide(int_t i)
{
    if (i == 3)
        return TEX152_BLOCK_ICON("dispenser_front.png");
    if (i == 0 || i == 1)
        return 62;
    return 45;
}

bool BlockDispenser::blockActivated(World *world, int_t i, int_t j, int_t k, EntityPlayer *entityplayer)
{
    if (world->multiplayerWorld) return true;
    TileEntityDispenser *tileentitydispenser = dynamic_cast<TileEntityDispenser*>(world->getBlockTileEntity(i, j, k));
    if (tileentitydispenser != nullptr)
        entityplayer->displayGUIDispenser(tileentitydispenser);
    return true;
}

void BlockDispenser::dispenseItem(World *world, int_t i, int_t j, int_t k, Random &random)
{
    TileEntityDispenser *dispenser = dynamic_cast<TileEntityDispenser*>(world->getBlockTileEntity(i, j, k));
    if (dispenser == nullptr)
        return;

    const int_t slot = dispenser->getRandomSlotFromInventory();
    if (slot < 0)
    {
        world->playAuxSFX(1001, i, j, k, 0);
        return;
    }

    ItemStack *stack = dispenser->getStackInSlot(slot);
    if (stack == nullptr || stack->stackSize <= 0)
    {
        world->playAuxSFX(1001, i, j, k, 0);
        return;
    }

    const int_t dir = world->getBlockMetadata(i, j, k) & 7;
    const int_t safeDir = (dir >= 0 && dir < 6) ? dir : 3;
    const int_t dx = FacingUtil::offsetsXForSide[safeDir];
    const int_t dy = FacingUtil::offsetsYForSide[safeDir];
    const int_t dz = FacingUtil::offsetsZForSide[safeDir];
    const int_t tx = i + dx;
    const int_t ty = j + dy;
    const int_t tz = k + dz;

    const double fx = (double)i + (double)dx * 0.7 + 0.5;
    const double fy = (double)j + (double)dy * 0.7 + 0.5;
    const double fz = (double)k + (double)dz * 0.7 + 0.5;
    const int_t smokeParam = encodeDispenserSmokeDir(safeDir);

    auto consumeOne = [&]()
    {
        stack->stackSize--;
        if (stack->stackSize <= 0)
            dispenser->setInventorySlotContents(slot, nullptr);
        else
            dispenser->onInventoryChanged();
    };

    // 1. Arrow
    if (Item::arrow != nullptr && stack->itemID == Item::arrow->shiftedIndex)
    {
        EntityArrow *entity = new EntityArrow(world, fx, fy, fz);
        entity->setArrowHeading((double)dx, (double)dy + 0.1, (double)dz, 1.1f, 6.0f);
        entity->doesArrowBelongToPlayer = true;
        if (!world->entityJoinedWorld(entity)) delete entity;
        consumeOne();
        world->playAuxSFX(1002, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 2. Egg
    if (Item::egg != nullptr && stack->itemID == Item::egg->shiftedIndex)
    {
        EntityEgg *entity = new EntityEgg(world, fx, fy, fz);
        entity->setThrowableHeading((double)dx, (double)dy + 0.1, (double)dz, 1.1f, 6.0f);
        if (!world->entityJoinedWorld(entity)) delete entity;
        consumeOne();
        world->playAuxSFX(1002, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 3. Snowball
    if (Item::snowball != nullptr && stack->itemID == Item::snowball->shiftedIndex)
    {
        EntitySnowball *entity = new EntitySnowball(world, fx, fy, fz);
        entity->setThrowableHeading((double)dx, (double)dy + 0.1, (double)dz, 1.1f, 6.0f);
        if (!world->entityJoinedWorld(entity)) delete entity;
        consumeOne();
        world->playAuxSFX(1002, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 4. Splash Potion
    if (Item::potion != nullptr && stack->itemID == Item::potion->shiftedIndex &&
        ItemPotion::isSplash(stack->getItemDamage()))
    {
        ItemStack *one = stack->copy();
        one->stackSize = 1;
        EntityPotion *entity = new EntityPotion(world, fx, fy, fz, one);
        delete one;
        entity->setThrowableHeading((double)dx, (double)dy + 0.1, (double)dz, 1.375f, 3.0f);
        if (!world->entityJoinedWorld(entity)) delete entity;
        consumeOne();
        world->playAuxSFX(1002, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 5. Bottle o' Enchanting
    if (Item::expBottle != nullptr && stack->itemID == Item::expBottle->shiftedIndex)
    {
        EntityExpBottle *entity = new EntityExpBottle(world, fx, fy, fz);
        entity->setThrowableHeading((double)dx, (double)dy + 0.1, (double)dz, 1.375f, 3.0f);
        if (!world->entityJoinedWorld(entity)) delete entity;
        consumeOne();
        world->playAuxSFX(1002, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 6. Spawn Egg
    if (Item::monsterPlacer != nullptr && stack->itemID == Item::monsterPlacer->shiftedIndex)
    {
        const double sx = (double)i + 0.5 + (double)dx;
        const double sy = (double)j + (double)dy + 0.2;
        const double sz = (double)k + 0.5 + (double)dz;
        ItemMonsterPlacer::spawnCreature(world, stack->getItemDamage(), sx, sy, sz);
        consumeOne();
        world->playAuxSFX(1000, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 7. Firework Rocket
    if (Item::firework != nullptr && stack->itemID == Item::firework->shiftedIndex)
    {
        const double rkx = (double)i + 0.5 + (double)dx;
        const double rky = (double)j + 0.5 + (double)dy;
        const double rkz = (double)k + 0.5 + (double)dz;
        ItemStack *one = stack->copy();
        one->stackSize = 1;
        EntityFireworkRocket *entity = new EntityFireworkRocket(world, rkx, rky, rkz, one);
        delete one;
        if (!world->entityJoinedWorld(entity)) delete entity;
        consumeOne();
        world->playAuxSFX(1002, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 8. Fire Charge
    if (Item::fireballCharge != nullptr && stack->itemID == Item::fireballCharge->shiftedIndex)
    {
        const double bx = fx + (double)dx * 0.3;
        const double by = fy + (double)dy * 0.3;
        const double bz = fz + (double)dz * 0.3;
        const double ax = (double)dx + random.nextGaussian() * 0.05;
        const double ay = (double)dy + random.nextGaussian() * 0.05;
        const double az = (double)dz + random.nextGaussian() * 0.05;
        EntitySmallFireball *entity = new EntitySmallFireball(world, bx, by, bz, ax, ay, az);
        if (!world->entityJoinedWorld(entity)) delete entity;
        consumeOne();
        world->playAuxSFX(1009, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 9. Boat
    if (Item::boat != nullptr && stack->itemID == Item::boat->shiftedIndex)
    {
        const double bx = (double)i + 0.5 + (double)dx * 1.125;
        const double by = (double)j + 0.5 + (double)dy * 1.125;
        const double bz = (double)k + 0.5 + (double)dz * 1.125;
        const Material *frontMat = world->getBlockMaterial(tx, ty, tz);
        double yOffset = 0.0;
        bool canPlaceBoat = false;
        if (frontMat == Material::water)
        {
            yOffset = 1.0;
            canPlaceBoat = true;
        }
        else if (frontMat == Material::air && world->getBlockMaterial(tx, ty - 1, tz) == Material::water)
        {
            yOffset = 0.0;
            canPlaceBoat = true;
        }
        if (canPlaceBoat)
        {
            EntityBoat *boat = new EntityBoat(world, bx, by + yOffset, bz);
            if (!world->entityJoinedWorld(boat)) delete boat;
            consumeOne();
            world->playAuxSFX(1000, i, j, k, 0);
            world->playAuxSFX(2000, i, j, k, smokeParam);
            return;
        }
        // Fallback to default drop if no water in front.
    }

    // 10. Water / Lava Bucket
    if ((Item::bucketWater != nullptr && stack->itemID == Item::bucketWater->shiftedIndex) ||
        (Item::bucketLava != nullptr && stack->itemID == Item::bucketLava->shiftedIndex))
    {
        ItemBucket *bucket = dynamic_cast<ItemBucket *>(stack->getItem());
        if (bucket != nullptr && bucket->tryPlaceContainedLiquid(world, (double)i, (double)j, (double)k, tx, ty, tz))
        {
            dispenser->setInventorySlotContents(slot, new ItemStack(Item::bucketEmpty));
            world->playAuxSFX(1000, i, j, k, 0);
            world->playAuxSFX(2000, i, j, k, smokeParam);
            return;
        }
        // Fallback to default drop if front block is solid.
    }

    // 11. Empty Bucket
    if (Item::bucketEmpty != nullptr && stack->itemID == Item::bucketEmpty->shiftedIndex)
    {
        const Material *frontMat = world->getBlockMaterial(tx, ty, tz);
        const int_t frontMeta = world->getBlockMetadata(tx, ty, tz);
        Item *filledItem = nullptr;
        if (frontMat == Material::water && frontMeta == 0)
            filledItem = Item::bucketWater;
        else if (frontMat == Material::lava && frontMeta == 0)
            filledItem = Item::bucketLava;

        if (filledItem != nullptr)
        {
            world->setBlockWithNotify(tx, ty, tz, 0);
            if (stack->stackSize == 1)
            {
                dispenser->setInventorySlotContents(slot, new ItemStack(filledItem));
            }
            else
            {
                stack->stackSize--;
                ItemStack *filled = new ItemStack(filledItem);
                if (dispenser->addItem(filled) < 0)
                {
                    dropItemFromDispenser(world, i, j, k, safeDir, filled, random);
                    dispenser->onInventoryChanged();
                    return;
                }
                dispenser->onInventoryChanged();
            }
            world->playAuxSFX(1000, i, j, k, 0);
            world->playAuxSFX(2000, i, j, k, smokeParam);
            return;
        }
        // Fallback to default drop if no source liquid in front.
    }

    // 12. Minecarts (Empty, Chest, Furnace, TNT, Hopper)
    if (ItemMinecart *itemCart = dynamic_cast<ItemMinecart *>(stack->getItem()))
    {
        const double cx = (double)i + 0.5 + (double)dx * 1.125;
        const double cy = (double)j + 0.5 + (double)dy * 1.125;
        const double cz = (double)k + 0.5 + (double)dz * 1.125;
        const int_t frontId = world->getBlockId(tx, ty, tz);
        double yOff = 0.0;
        bool canPlaceCart = false;
        if (BlockRail::isRailBlock(frontId))
        {
            yOff = 0.0;
            canPlaceCart = true;
        }
        else if (frontId == 0 && BlockRail::isRailBlock(world->getBlockId(tx, ty - 1, tz)))
        {
            yOff = -1.0;
            canPlaceCart = true;
        }
        if (canPlaceCart)
        {
            EntityMinecart *cart = EntityMinecart::createMinecart(world, cx, cy + yOff, cz, itemCart->minecartType);
            if (cart != nullptr && !world->entityJoinedWorld(cart))
                delete cart;
            consumeOne();
            world->playAuxSFX(1000, i, j, k, 0);
            world->playAuxSFX(2000, i, j, k, smokeParam);
            return;
        }
        // Fallback to default drop if no rail.
    }

    // 13. TNT Block -> Primed TNT
    if (Block::tnt != nullptr && stack->itemID == Block::tnt->blockID)
    {
        EntityTNTPrimed *tnt = new EntityTNTPrimed(world, (double)tx + 0.5, (double)ty + 0.5, (double)tz + 0.5);
        if (!world->entityJoinedWorld(tnt)) delete tnt;
        world->playSoundEffect((double)tx + 0.5, (double)ty + 0.5, (double)tz + 0.5, "random.fuse", 1.0f, 1.0f);
        consumeOne();
        world->playAuxSFX(1000, i, j, k, 0);
        world->playAuxSFX(2000, i, j, k, smokeParam);
        return;
    }

    // 14. Flint and Steel
    if (Item::flintAndSteel != nullptr && stack->itemID == Item::flintAndSteel->shiftedIndex)
    {
        bool lit = false;
        if (world->isAirBlock(tx, ty, tz))
        {
            world->setBlockWithNotify(tx, ty, tz, Block::fire->blockID);
            lit = true;
        }
        else if (Block::tnt != nullptr && world->getBlockId(tx, ty, tz) == Block::tnt->blockID)
        {
            world->setBlockWithNotify(tx, ty, tz, 0);
            EntityTNTPrimed *tnt = new EntityTNTPrimed(world, (double)tx + 0.5, (double)ty + 0.5, (double)tz + 0.5);
            if (!world->entityJoinedWorld(tnt)) delete tnt;
            world->playSoundEffect((double)tx + 0.5, (double)ty + 0.5, (double)tz + 0.5, "random.fuse", 1.0f, 1.0f);
            lit = true;
        }
        if (lit)
        {
            const int_t nextDmg = stack->getItemDamage() + 1;
            if (nextDmg > stack->getMaxDamage())
                dispenser->setInventorySlotContents(slot, nullptr);
            else
            {
                stack->setItemDamage(nextDmg);
                dispenser->onInventoryChanged();
            }
            world->playAuxSFX(1000, i, j, k, 0);
            world->playAuxSFX(2000, i, j, k, smokeParam);
        }
        else
        {
            world->playAuxSFX(1001, i, j, k, 0);
        }
        return;
    }

    // 15. Bonemeal
    if (Item::dyePowder != nullptr && stack->itemID == Item::dyePowder->shiftedIndex &&
        stack->getItemDamage() == 15)
    {
        if (ItemDye::applyBonemeal(stack, world, tx, ty, tz))
        {
            if (stack->stackSize <= 0)
                dispenser->setInventorySlotContents(slot, nullptr);
            else
                dispenser->onInventoryChanged();
            world->playAuxSFX(2005, tx, ty, tz, 0);
            world->playAuxSFX(1000, i, j, k, 0);
            world->playAuxSFX(2000, i, j, k, smokeParam);
        }
        else
        {
            world->playAuxSFX(1001, i, j, k, 0);
        }
        return;
    }

    // 16. Armor -> equip onto player standing in front
    if (ItemArmor *armor = dynamic_cast<ItemArmor *>(stack->getItem()))
    {
        AxisAlignedBB *box = AxisAlignedBB::getBoundingBoxFromPool(
            (double)tx, (double)ty, (double)tz,
            (double)(tx + 1), (double)(ty + 1), (double)(tz + 1));
        const int_t armorSlot = 3 - armor->armorType;
        if (armorSlot >= 0 && armorSlot < 4)
        {
            auto tryEquipPlayer = [&](EntityPlayer *player) -> bool
            {
                if (player != nullptr && !player->isDead && player->inventory != nullptr &&
                    player->inventory->armorInventory[armorSlot] == nullptr)
                {
                    ItemStack *equipped = stack->copy();
                    equipped->stackSize = 1;
                    player->inventory->armorInventory[armorSlot] = equipped;
                    player->inventory->onInventoryChanged();
                    consumeOne();
                    world->playAuxSFX(1000, i, j, k, 0);
                    world->playAuxSFX(2000, i, j, k, smokeParam);
                    return true;
                }
                return false;
            };

            for (EntityPlayer *player : world->playerEntities)
            {
                if (player != nullptr && player->boundingBox != nullptr &&
                    player->boundingBox->intersectsWith(box) && tryEquipPlayer(player))
                {
                    return;
                }
            }

            std::vector<Entity *> entities = world->getEntitiesWithinAABBExcludingEntity(nullptr, box);
            for (Entity *e : entities)
            {
                EntityPlayer *player = dynamic_cast<EntityPlayer *>(e);
                if (tryEquipPlayer(player))
                    return;
            }
        }
        // Fallback to default drop if no unarmored player in front.
    }

    // Default behavior: dispense item as EntityItem
    ItemStack *popped = dispenser->decrStackSize(slot, 1);
    if (popped != nullptr)
    {
        dropItemFromDispenser(world, i, j, k, safeDir, popped, random);
    }
}

void BlockDispenser::onNeighborBlockChange(World *world, int_t i, int_t j, int_t k, int_t l)
{
    (void)l;
    const bool powered = world->isBlockIndirectlyGettingPowered(i, j, k) ||
                         world->isBlockIndirectlyGettingPowered(i, j + 1, k);
    const int_t meta = world->getBlockMetadata(i, j, k);
    const bool wasPowered = (meta & 8) != 0;
    if (powered && !wasPowered)
    {
        world->scheduleBlockUpdate(i, j, k, blockID, tickRate());
        world->setBlockMetadataWithNotify152(i, j, k, meta | 8, 4);
    }
    else if (!powered && wasPowered)
    {
        world->setBlockMetadataWithNotify152(i, j, k, meta & ~8, 4);
    }
}

void BlockDispenser::updateTick(World *world, int_t i, int_t j, int_t k, Random &random)
{
    if (!world->multiplayerWorld)
        dispenseItem(world, i, j, k, random);
}

TileEntity *BlockDispenser::getBlockEntity() { return new TileEntityDispenser(); }

void BlockDispenser::onBlockPlacedBy(World *world, int_t i, int_t j, int_t k, EntityLiving *entityliving)
{
    const int_t orientation = BlockPistonBase::getOrientationFromEntity(world, i, j, k, entityliving);
    world->setBlockMetadataWithNotify152(i, j, k, orientation, 2);
}

void BlockDispenser::onBlockRemoval(World *world, int_t i, int_t j, int_t k)
{
    TileEntityDispenser *tileentitydispenser = dynamic_cast<TileEntityDispenser*>(world->getBlockTileEntity(i, j, k));
    if (tileentitydispenser == nullptr)
    {
        BlockContainer::onBlockRemoval(world, i, j, k);
        return;
    }
    for (int_t l = 0; l < tileentitydispenser->getSizeInventory(); l++)
    {
        ItemStack *itemstack = tileentitydispenser->getStackInSlot(l);
        if (itemstack == nullptr) continue;
        float f  = random.nextFloat() * 0.8f + 0.1f;
        float f1 = random.nextFloat() * 0.8f + 0.1f;
        float f2 = random.nextFloat() * 0.8f + 0.1f;
        while (itemstack->stackSize > 0)
        {
            int_t i1 = random.nextInt(21) + 10;
            if (i1 > itemstack->stackSize) i1 = itemstack->stackSize;
            itemstack->stackSize -= i1;
            ItemStack *droppedStack = new ItemStack(itemstack->itemID, i1, itemstack->getItemDamage());
            if (itemstack->hasTagCompound())
            {
                NBTTagCompound *tagCopy = dynamic_cast<NBTTagCompound *>(itemstack->getTagCompound()->copy());
                droppedStack->setTagCompound(tagCopy);
            }
            EntityItem *entityitem = new EntityItem(world,
                (float)i + f, (float)j + f1, (float)k + f2, droppedStack);
            float f3 = 0.05f;
            entityitem->motionX = (float)random.nextGaussian() * f3;
            entityitem->motionY = (float)random.nextGaussian() * f3 + 0.2f;
            entityitem->motionZ = (float)random.nextGaussian() * f3;
            if (!world->entityJoinedWorld(entityitem))
                delete entityitem;
        }
    }
    world->func_96440_m(i, j, k, blockID);
    BlockContainer::onBlockRemoval(world, i, j, k);
}

bool BlockDispenser::hasComparatorInputOverride()
{
    return true;
}

int_t BlockDispenser::getComparatorInputOverride(World *world, int_t x, int_t y, int_t z, int_t side)
{
    (void)side;
    return Container::calcRedstoneFromInventory(dynamic_cast<IInventory *>(world->getBlockTileEntity(x, y, z)));
}
