#include "EntityChocobo.h"
#include "ChocoCraftItems.h"
#include "GuiChocoboMenu.h"
#include "mods/ModManager.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/Block.h"
#include "net/minecraft/src/Item.h"
#include "net/minecraft/src/ItemStack.h"
#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/MovementInput.h"
#include "net/minecraft/src/EntityItem.h"
#include "net/minecraft/src/InventoryPlayer.h"
#include "net/minecraft/src/NBTTagCompound.h"
#include "net/minecraft/src/NBTTagList.h"
#include "net/minecraft/src/MathHelper.h"
#include "net/minecraft/src/Potion.h"
#include "net/minecraft/src/PotionEffect.h"
#include "net/minecraft/src/AxisAlignedBB.h"
#include "platform/Resources.h"

#include <algorithm>
#include <cmath>

namespace
{
bool hasChocoboKwehSound()
{
    static int s_state = -1;
    if (s_state < 0)
    {
        s_state = PlatformResources::resolveExisting("resources/newsound/mob/chocobo/kweh.ogg").empty() ? 0 : 1;
    }
    return s_state == 1;
}

void consumeHeldItem(EntityPlayer *player)
{
    if (player == nullptr || player->capabilities.isCreativeMode || player->inventory == nullptr)
        return;
    ItemStack *held = player->inventory->getCurrentItem();
    if (held == nullptr)
        return;
    held->stackSize--;
    if (held->stackSize <= 0)
        player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
}
} // namespace

// ============================================================================
// InventoryChocoboSaddlebag (27 slots for Saddlebags, 54 slots for Pack Bags)
// ============================================================================

InventoryChocoboSaddlebag::InventoryChocoboSaddlebag(const std::string &title, int_t slotCount)
    : m_title(title)
    , m_slotCount(std::clamp<int_t>(slotCount, SADDLEBAG_SLOTS, MAX_SLOTS))
{
    for (int_t i = 0; i < MAX_SLOTS; ++i)
        m_slots[i] = nullptr;
}

InventoryChocoboSaddlebag::~InventoryChocoboSaddlebag()
{
    for (int_t i = 0; i < MAX_SLOTS; ++i)
    {
        delete m_slots[i];
        m_slots[i] = nullptr;
    }
}

void InventoryChocoboSaddlebag::setSlotCount(int_t count)
{
    m_slotCount = std::clamp<int_t>(count, SADDLEBAG_SLOTS, MAX_SLOTS);
}

ItemStack *InventoryChocoboSaddlebag::getStackInSlot(int_t slot)
{
    if (slot < 0 || slot >= m_slotCount)
        return nullptr;
    return m_slots[slot];
}

ItemStack *InventoryChocoboSaddlebag::decrStackSize(int_t slot, int_t amount)
{
    if (slot < 0 || slot >= m_slotCount || m_slots[slot] == nullptr)
        return nullptr;
    if (m_slots[slot]->stackSize <= amount)
    {
        ItemStack *res = m_slots[slot];
        m_slots[slot] = nullptr;
        return res;
    }
    ItemStack *split = m_slots[slot]->splitStack(amount);
    if (m_slots[slot]->stackSize == 0)
    {
        delete m_slots[slot];
        m_slots[slot] = nullptr;
    }
    return split;
}

ItemStack *InventoryChocoboSaddlebag::getStackInSlotOnClosing(int_t slot)
{
    (void)slot;
    return nullptr;
}

void InventoryChocoboSaddlebag::setInventorySlotContents(int_t slot, ItemStack *stack)
{
    if (slot < 0 || slot >= m_slotCount)
        return;
    if (m_slots[slot] != stack)
    {
        delete m_slots[slot];
        m_slots[slot] = stack;
    }
    if (stack != nullptr && stack->stackSize > getInventoryStackLimit())
        stack->stackSize = getInventoryStackLimit();
}

bool InventoryChocoboSaddlebag::canInteractWith(EntityPlayer *player)
{
    return player != nullptr && !player->isDead;
}

void InventoryChocoboSaddlebag::writeToNBT(NBTTagCompound *nbt) const
{
    NBTTagList *list = new NBTTagList();
    for (int_t i = 0; i < MAX_SLOTS; ++i)
    {
        if (m_slots[i] != nullptr)
        {
            NBTTagCompound *slotTag = new NBTTagCompound();
            slotTag->setByte("Slot", static_cast<byte_t>(i));
            m_slots[i]->writeToNBT(slotTag);
            list->setTag(slotTag);
        }
    }
    nbt->setTag("SaddlebagItems", list);
}

void InventoryChocoboSaddlebag::readFromNBT(NBTTagCompound *nbt)
{
    for (int_t i = 0; i < MAX_SLOTS; ++i)
    {
        delete m_slots[i];
        m_slots[i] = nullptr;
    }
    NBTTagList *list = nbt->getTagList("SaddlebagItems");
    if (list == nullptr)
        return;
    for (int_t i = 0; i < list->tagCount(); ++i)
    {
        NBTTagCompound *slotTag = dynamic_cast<NBTTagCompound *>(list->tagAt(i));
        if (slotTag == nullptr)
            continue;
        int_t slot = static_cast<unsigned char>(slotTag->getByte("Slot"));
        if (slot >= 0 && slot < MAX_SLOTS)
            m_slots[slot] = ItemStack::loadItemStackFromNBT(slotTag);
    }
}

void InventoryChocoboSaddlebag::dropAllContents(World *world, double x, double y, double z)
{
    if (world == nullptr || world->multiplayerWorld)
        return;
    for (int_t i = 0; i < MAX_SLOTS; ++i)
    {
        if (m_slots[i] != nullptr)
        {
            EntityItem *drop = new EntityItem(world, x, y + 0.5, z, m_slots[i]);
            m_slots[i] = nullptr;
            drop->delayBeforeCanPickup = 10;
            world->entityJoinedWorld(drop);
        }
    }
}

// ============================================================================
// EntityChocobo (Port of ChocoCraft 4.1.5 — 1.7.10)
// ============================================================================

EntityChocobo::EntityChocobo(World *world)
    : EntityAnimal(world)
    , m_color(ChocoboColor::Yellow)
    , m_isMale(rand.nextInt(100) < 50)
    , m_tamed(false)
    , m_saddled(false)
    , m_saddlebags(false)
    , m_packbags(false)
    , m_hidename(false)
    , m_fedGold(false)
    , m_wasChild(false)
    , m_flying(false)
    , m_isHighJumping(false)
    , m_gliding(false)
    , m_gearDropped(false)
    , m_aiMode(ChocoboAiMode::Wander)
    , m_inventory("Chocobo Saddlebags", InventoryChocoboSaddlebag::SADDLEBAG_SLOTS)
    , m_wingRotation(0.0f)
    , m_destPos(0.0f)
    , m_wingRotDelta(1.0f)
    , m_timeUntilNextFeather(600 + rand.nextInt(600))
    , m_mountGraceTicks(0)
    , m_flyParticleCooldown(0)
{
    ensureEntityInit();
    texture = "/chococraft/textures/entities/Chocobos/Untamed/Male/yellowchocobo.png";
    setSize(0.95f, 1.85f);
    moveSpeed = 0.65f;
    stepHeight = 0.5f;
    setHealth(getMaxHealth());
}

bool EntityChocobo::canClimb() const
{
    return m_color == ChocoboColor::Green || m_color == ChocoboColor::White ||
           m_color == ChocoboColor::Black || m_color == ChocoboColor::Gold ||
           m_color == ChocoboColor::Pink  || m_color == ChocoboColor::Red ||
           m_color == ChocoboColor::Purple;
}

bool EntityChocobo::canCrossWater() const
{
    return m_color == ChocoboColor::Blue  || m_color == ChocoboColor::White ||
           m_color == ChocoboColor::Black || m_color == ChocoboColor::Gold ||
           m_color == ChocoboColor::Pink  || m_color == ChocoboColor::Red;
}

bool EntityChocobo::canJumpHigh() const
{
    return m_color == ChocoboColor::Green || m_color == ChocoboColor::White ||
           m_color == ChocoboColor::Black;
}

bool EntityChocobo::canFly() const
{
    return m_color == ChocoboColor::Gold || m_color == ChocoboColor::Pink ||
           m_color == ChocoboColor::Red  || m_color == ChocoboColor::Purple;
}

bool EntityChocobo::canBreatheUnderwater()
{
    return m_color == ChocoboColor::Blue || m_color == ChocoboColor::Gold ||
           m_color == ChocoboColor::Pink || m_color == ChocoboColor::Red;
}

void EntityChocobo::setColor(ChocoboColor color)
{
    m_color = color;
    immuneToFire = (m_color == ChocoboColor::Purple);
    applyMountedStepHeight(riddenByEntity != nullptr);
    if (getHealth() > getMaxHealth())
        setHealth(getMaxHealth());
}

void EntityChocobo::setSaddled(bool saddled)
{
    m_saddled = saddled;
    if (saddled)
    {
        m_packbags = false;
        m_inventory.setSlotCount(InventoryChocoboSaddlebag::SADDLEBAG_SLOTS);
    }
}

void EntityChocobo::setSaddlebags(bool bags)
{
    m_saddlebags = bags;
    if (bags)
    {
        m_packbags = false;
        m_inventory.setSlotCount(InventoryChocoboSaddlebag::SADDLEBAG_SLOTS);
    }
}

void EntityChocobo::setPackbags(bool pack)
{
    m_packbags = pack;
    if (pack)
    {
        m_saddled = false;
        m_saddlebags = false;
        m_inventory.setSlotCount(InventoryChocoboSaddlebag::PACKBAG_SLOTS);
    }
}

void EntityChocobo::setCustomName(const std::string &name)
{
    m_customName = name;
    m_hidename = false;
    if (!m_customName.empty())
        m_inventory.setTitle(m_customName);
    else
        m_inventory.setTitle(m_packbags ? "Chocobo Pack Bags" : "Chocobo Saddlebags");
}

const char *EntityChocobo::getColorName(ChocoboColor color, bool spanish)
{
    switch (color)
    {
    case ChocoboColor::Yellow: return spanish ? "Amarillo" : "Yellow";
    case ChocoboColor::Green:  return spanish ? "Verde"    : "Green";
    case ChocoboColor::Blue:   return spanish ? "Azul"     : "Blue";
    case ChocoboColor::White:  return spanish ? "Blanco"   : "White";
    case ChocoboColor::Black:  return spanish ? "Negro"    : "Black";
    case ChocoboColor::Gold:   return spanish ? "Dorado"   : "Gold";
    case ChocoboColor::Pink:   return spanish ? "Rosa"     : "Pink";
    case ChocoboColor::Red:    return spanish ? "Rojo"     : "Red";
    case ChocoboColor::Purple: return spanish ? "Morado"   : "Purple";
    default:                   return spanish ? "Amarillo" : "Yellow";
    }
}

const char *EntityChocobo::getColorAbility(ChocoboColor color, bool spanish)
{
    switch (color)
    {
    case ChocoboColor::Yellow: return spanish ? "Corredor clasico" : "Classic runner";
    case ChocoboColor::Green:  return spanish ? "Escala montanas y salta alto" : "Climbs mountains & high jump";
    case ChocoboColor::Blue:   return spanish ? "Nada rapido + Resp. acuatica" : "Fast swim + Water Breathing";
    case ChocoboColor::White:  return spanish ? "Escala, nada y gran salto" : "Climbs, swims & high jump";
    case ChocoboColor::Black:  return spanish ? "Veloz + Vision nocturna" : "Swift + Night Vision";
    case ChocoboColor::Gold:   return spanish ? "Vuela (A/L3), escala y nada" : "Flies (A/L3), climbs & swims";
    case ChocoboColor::Pink:   return spanish ? "Vuelo supersonico, escala y nada" : "Fastest flyer, climbs & swims";
    case ChocoboColor::Red:    return spanish ? "Vuelo supersonico, escala y nada" : "Fastest flyer, climbs & swims";
    case ChocoboColor::Purple: return spanish ? "Vuela + Inmune fuego/lava" : "Flies + Fire/Lava immune";
    default:                   return "";
    }
}

const char *EntityChocobo::getColorTextureFile(ChocoboColor color, bool isChicobo)
{
    switch (color)
    {
    case ChocoboColor::Yellow: return isChicobo ? "chocobo.png" : "yellowchocobo.png";
    case ChocoboColor::Green:  return "greenchocobo.png";
    case ChocoboColor::Blue:   return "bluechocobo.png";
    case ChocoboColor::White:  return "whitechocobo.png";
    case ChocoboColor::Black:  return "blackchocobo.png";
    case ChocoboColor::Gold:   return "goldchocobo.png";
    case ChocoboColor::Pink:   return "pinkchocobo.png";
    case ChocoboColor::Red:    return "redchocobo.png";
    case ChocoboColor::Purple: return "purplechocobo.png";
    default:                   return isChicobo ? "chocobo.png" : "yellowchocobo.png";
    }
}

std::string EntityChocobo::getChocoboTexturePath()
{
    if (isChild())
    {
        std::string path = "/chococraft/textures/entities/Chicobos/";
        if (m_tamed)
            path += "Tamed/";
        path += getColorTextureFile(m_color, true);
        return path;
    }

    std::string path = "/chococraft/textures/entities/Chocobos/";
    if (m_packbags)
        path += "PackBagged/";
    else if (m_saddlebags)
        path += "SaddleBagged/";
    else if (m_saddled)
        path += "Saddled/";
    else if (m_tamed)
        path += "Tamed/";
    else
        path += "Untamed/";

    path += m_isMale ? "Male/" : "Female/";
    path += getColorTextureFile(m_color, false);
    return path;
}

int_t EntityChocobo::getMaxHealth() const
{
    if (m_wasChild)
    {
        switch (m_color)
        {
        case ChocoboColor::White:
        case ChocoboColor::Black:  return 15;
        case ChocoboColor::Gold:
        case ChocoboColor::Pink:
        case ChocoboColor::Red:
        case ChocoboColor::Purple: return 20;
        default:                   return 10;
        }
    }
    switch (m_color)
    {
    case ChocoboColor::Gold:
    case ChocoboColor::Pink:
    case ChocoboColor::Red:
    case ChocoboColor::Purple: return 50;
    case ChocoboColor::White:
    case ChocoboColor::Black:  return 40;
    default:                   return 30;
    }
}

float EntityChocobo::getRideSpeed() const
{
    switch (m_color)
    {
    case ChocoboColor::Pink:
    case ChocoboColor::Red:    return 0.44f;
    case ChocoboColor::Gold:   return 0.41f;
    case ChocoboColor::Black:
    case ChocoboColor::Purple: return 0.36f;
    case ChocoboColor::White:  return 0.33f;
    case ChocoboColor::Green:
    case ChocoboColor::Blue:   return 0.29f;
    default:                   return 0.25f;
    }
}

float EntityChocobo::getWaterRideSpeed() const
{
    switch (m_color)
    {
    case ChocoboColor::Blue:   return 0.44f;
    case ChocoboColor::White:  return 0.38f;
    case ChocoboColor::Pink:
    case ChocoboColor::Red:    return 0.30f;
    case ChocoboColor::Gold:
    case ChocoboColor::Black:  return 0.26f;
    default:                   return 0.14f;
    }
}

float EntityChocobo::getAirRideSpeed() const
{
    switch (m_color)
    {
    case ChocoboColor::Pink:
    case ChocoboColor::Red:    return 0.46f;
    case ChocoboColor::Gold:
    case ChocoboColor::Purple: return 0.42f;
    default:                   return getRideSpeed() * 0.85f;
    }
}

float EntityChocobo::getJumpVelocity() const
{
    if (canFly())
        return 0.62f;
    if (canJumpHigh())
        return 0.78f;
    return 0.52f;
}

void EntityChocobo::applyMountedStepHeight(bool mounted)
{
    if (!mounted)
    {
        stepHeight = 0.5f;
        return;
    }
    if (m_color == ChocoboColor::Green || m_color == ChocoboColor::White ||
        m_color == ChocoboColor::Black || m_color == ChocoboColor::Gold ||
        m_color == ChocoboColor::Pink  || m_color == ChocoboColor::Red)
        stepHeight = 2.0f;
    else
        stepHeight = 1.0f;
}

void EntityChocobo::applyRiderBuffs(EntityPlayer *rider)
{
    if (rider == nullptr || worldObj == nullptr || worldObj->multiplayerWorld)
        return;
    if (m_color == ChocoboColor::Blue && Potion::waterBreathing != nullptr)
        rider->addPotionEffect(new PotionEffect(Potion::waterBreathing->id, 100, 0));
    else if (m_color == ChocoboColor::Black && Potion::nightVision != nullptr)
        rider->addPotionEffect(new PotionEffect(Potion::nightVision->id, 220, 0));
    else if (m_color == ChocoboColor::Purple && Potion::fireResistance != nullptr)
    {
        rider->addPotionEffect(new PotionEffect(Potion::fireResistance->id, 100, 0));
        rider->extinguish();
    }
}

jstring EntityChocobo::getLivingSound()
{
    if (hasChocoboKwehSound())
        return "mob.chocobo.kweh";
    return "mob.chicken";
}

jstring EntityChocobo::getHurtSound()
{
    if (hasChocoboKwehSound())
        return "mob.chocobo.kweh";
    return "mob.chickenhurt";
}

jstring EntityChocobo::getDeathSound()
{
    if (hasChocoboKwehSound())
        return "mob.chocobo.kweh";
    return "mob.chickenhurt";
}

float EntityChocobo::getSoundPitch()
{
    const float base = isChild() ? 1.8f : 1.0f;
    return base + (rand.nextFloat() - rand.nextFloat()) * 0.2f;
}

void EntityChocobo::spawnParticles(const char *name, int count)
{
    if (worldObj == nullptr)
        return;
    for (int i = 0; i < count; ++i)
    {
        double ox = (rand.nextDouble() - 0.5) * 1.2;
        double oy = rand.nextDouble() * 1.4 + 0.3;
        double oz = (rand.nextDouble() - 0.5) * 1.2;
        worldObj->spawnParticle(name, posX + ox, posY + oy, posZ + oz, 0.0, 0.05, 0.0);
    }
}

void EntityChocobo::mountChocobo(EntityPlayer *player)
{
    if (player == nullptr || !m_saddled || m_packbags || isChild())
        return;
    player->setSprinting(false);
    player->fallDistance = 0.0f;
    fallDistance = 0.0f;
    setPathToEntity(nullptr);
    playerToAttack = nullptr;
    moveForward = 0.0f;
    moveStrafing = 0.0f;
    randomYawVelocity = 0.0f;
    isJumping = false;
    m_flying = false;
    m_isHighJumping = false;
    m_mountGraceTicks = 12;
    applyMountedStepHeight(true);
    player->mountEntity(this);
}

void EntityChocobo::dismountChocobo(EntityPlayer *player)
{
    isJumping = false;
    m_flying = false;
    m_isHighJumping = false;
    m_gliding = false;
    fallDistance = 0.0f;
    applyMountedStepHeight(false);
    if (player != nullptr)
    {
        player->fallDistance = 0.0f;
        player->motionY = 0.0;
        player->mountEntity(nullptr);
        // Place rider safely at ground height if dismounting on ground so they never take fall damage
        if (onGround && worldObj != nullptr)
        {
            const float sideRad = (rotationYaw + 90.0f) * 3.14159265f / 180.0f;
            const double sideX = posX - std::sin(sideRad) * 1.15;
            const double sideZ = posZ + std::cos(sideRad) * 1.15;
            const int_t bx = MathHelper::floor_double(sideX);
            const int_t by = MathHelper::floor_double(boundingBox->minY);
            const int_t bz = MathHelper::floor_double(sideZ);
            if (!worldObj->isBlockNormalCube(bx, by, bz) && !worldObj->isBlockNormalCube(bx, by + 1, bz))
                player->setLocationAndAngles(sideX, boundingBox->minY + 0.1, sideZ, player->rotationYaw, player->rotationPitch);
            else
                player->setLocationAndAngles(posX, boundingBox->minY + 0.1, posZ, player->rotationYaw, player->rotationPitch);
        }
        player->fallDistance = 0.0f;
        player->onGround = onGround;
    }
}

bool EntityChocobo::teleportToOwner(EntityPlayer *owner)
{
    if (owner == nullptr || worldObj == nullptr)
        return false;
    const int_t baseX = MathHelper::floor_double(owner->posX) - 2;
    const int_t baseY = MathHelper::floor_double(owner->boundingBox->minY);
    const int_t baseZ = MathHelper::floor_double(owner->posZ) - 2;
    for (int_t ox = 0; ox <= 4; ++ox)
    {
        for (int_t oz = 0; oz <= 4; ++oz)
        {
            if (ox >= 1 && oz >= 1 && ox <= 3 && oz <= 3)
                continue;
            const int_t tx = baseX + ox;
            const int_t tz = baseZ + oz;
            if (worldObj->isBlockNormalCube(tx, baseY - 1, tz) &&
                !worldObj->isBlockNormalCube(tx, baseY, tz) &&
                !worldObj->isBlockNormalCube(tx, baseY + 1, tz))
            {
                setLocationAndAngles(static_cast<double>(tx) + 0.5, static_cast<double>(baseY),
                                     static_cast<double>(tz) + 0.5, rotationYaw, rotationPitch);
                setPathToEntity(nullptr);
                fallDistance = 0.0f;
                return true;
            }
        }
    }
    return false;
}

void EntityChocobo::updateRiderPosition()
{
    if (riddenByEntity == nullptr)
        return;
    // ChocoCraft 4.1.5 EntityChocoboRideable.updateRiderPosition
    const float rad = (rotationYaw - 90.0f) * 3.14159265f / 180.0f;
    const double deltaX = static_cast<double>(MathHelper::cos(rad) * 0.4f);
    const double deltaZ = static_cast<double>(MathHelper::sin(rad) * 0.4f);
    riddenByEntity->setPosition(posX + deltaX, posY + getMountedYOffset() + riddenByEntity->getYOffset(), posZ + deltaZ);
    riddenByEntity->fallDistance = 0.0f;
}

void EntityChocobo::fall(float dist)
{
    // Flyers (Gold/Pink/Red/Purple), Black, mounted chocobos, and gliding chocobos NEVER take fall damage!
    if (riddenByEntity != nullptr)
        riddenByEntity->fallDistance = 0.0f;

    if (canFly() || m_color == ChocoboColor::Black || riddenByEntity != nullptr || m_gliding)
    {
        fallDistance = 0.0f;
        return;
    }

    // ChocoCraft 4.1.5: chocobos glide gently and only take damage above 6 blocks when wild/unmounted
    int_t dmg = static_cast<int_t>(std::ceil(static_cast<double>(dist - 6.0f)));
    if (dmg > 0)
        EntityAnimal::fall(static_cast<float>(dmg) + 3.0f);
    fallDistance = 0.0f;
}

void EntityChocobo::growUpToAdult()
{
    setGrowingAge(0);
    m_wasChild = false;
    setSize(0.95f, 1.85f);
    setHealth(getMaxHealth());
    spawnParticles("explode", 10);
    if (worldObj != nullptr)
        worldObj->playSoundAtEntity(this, getLivingSound(), 0.9f, 1.0f);
}

void EntityChocobo::doStrawHealing()
{
    if (worldObj == nullptr || worldObj->multiplayerWorld || !m_tamed)
        return;
    if (getHealth() >= getMaxHealth() || (worldObj->getWorldTime() % 40LL) != 0LL)
        return;
    if (rand.nextInt(100) >= 20)
        return;
    const int_t bx = MathHelper::floor_double(posX);
    const int_t by = MathHelper::floor_double(boundingBox->minY);
    const int_t bz = MathHelper::floor_double(posZ);
    if (worldObj->getBlockId(bx, by, bz) != ChocoCraftItems::BLOCK_STRAW_ID)
        return;
    // Look for a filled cauldron within 6 blocks (ChocoCraft 4.1.5 pen healing)
    for (int_t dy = -2; dy <= 2; ++dy)
    {
        for (int_t dx = -6; dx <= 6; ++dx)
        {
            for (int_t dz = -6; dz <= 6; ++dz)
            {
                if (Block::cauldron != nullptr &&
                    worldObj->getBlockId(bx + dx, by + dy, bz + dz) == Block::cauldron->blockID &&
                    worldObj->getBlockMetadata(bx + dx, by + dy, bz + dz) > 0)
                {
                    heal(rand.nextInt(3) + 1);
                    spawnParticles("heart", 2);
                    return;
                }
            }
        }
    }
}

void EntityChocobo::updatePlayerActionState()
{
    // CRITICAL: When ridden, ALL autonomous AI is 100% disabled!
    // The chocobo obeys ONLY the player's controls.
    if (riddenByEntity != nullptr)
    {
        setPathToEntity(nullptr);
        playerToAttack = nullptr;
        moveForward = 0.0f;
        moveStrafing = 0.0f;
        randomYawVelocity = 0.0f;
        isJumping = false;
        return;
    }

    // Tamed Stay mode (or saddled/packbagged chocobos not set to Follow/Wander, and not breeding)
    if (m_tamed && m_aiMode == ChocoboAiMode::Stay && !isInLove())
    {
        setPathToEntity(nullptr);
        playerToAttack = nullptr;
        moveForward = 0.0f;
        moveStrafing = 0.0f;
        randomYawVelocity = 0.0f;
        isJumping = false;
        return;
    }

    // Tamed Follow mode
    if (m_tamed && m_aiMode == ChocoboAiMode::Follow && worldObj != nullptr && !isInLove())
    {
        EntityPlayer *owner = nullptr;
        for (EntityPlayer *p : worldObj->playerEntities)
        {
            if (p != nullptr && !p->isDead && (m_ownerName.empty() || std::string(p->username) == m_ownerName))
            {
                owner = p;
                break;
            }
        }
        if (owner != nullptr)
        {
            const double distSq = getDistanceSqToEntity(owner);
            if (distSq > 22.0 * 22.0 && distSq < 96.0 * 96.0)
            {
                teleportToOwner(owner);
                return;
            }
            if (distSq > 12.0)
            {
                const double dx = owner->posX - posX;
                const double dz = owner->posZ - posZ;
                const double dist = std::sqrt(dx * dx + dz * dz);
                if (dist > 0.1)
                {
                    rotationYaw = static_cast<float>(std::atan2(dz, dx) * 180.0 / 3.14159265) - 90.0f;
                    rotationYawHead = rotationYaw;
                    moveForward = moveSpeed;
                    moveStrafing = 0.0f;
                    isJumping = (isCollidedHorizontally && onGround) || isInWater();
                }
                return;
            }
            // Close enough to owner: stand calmly nearby
            moveForward = 0.0f;
            moveStrafing = 0.0f;
            isJumping = isInWater();
            return;
        }
    }

    // Wild or Wander mode (or breeding love mode)
    EntityAnimal::updatePlayerActionState();
}

void EntityChocobo::onLivingUpdate()
{
    // Sync hitbox size when Chicobo grows into adult Chocobo (or loaded as baby)
    const bool childNow = isChild();
    if (childNow != m_wasChild)
    {
        m_wasChild = childNow;
        if (childNow)
            setSize(0.5f, 0.5f);
        else
            growUpToAdult();
    }

    if (m_mountGraceTicks > 0)
        --m_mountGraceTicks;
    if (m_flyParticleCooldown > 0)
        --m_flyParticleCooldown;

    // Purple chocobo never burns
    if (m_color == ChocoboColor::Purple)
    {
        extinguish();
        immuneToFire = true;
    }

    // ChocoCraft 4.1.5 wing animation & gentle gliding physics
    m_destPos += static_cast<float>(onGround ? -1 : 4) * 0.3f;
    m_destPos = std::clamp(m_destPos, 0.0f, 1.0f);
    if (!onGround && m_wingRotDelta < 1.0f)
        m_wingRotDelta = 1.0f;
    m_wingRotDelta *= 0.9f;
    m_wingRotation += m_wingRotDelta * 2.0f;

    if (onGround)
    {
        m_flying = false;
        m_isHighJumping = false;
        m_gliding = false;
    }

    // Periodic feather shedding for adult chocobos (ChocoCraft 4.1.5)
    if (!childNow && worldObj != nullptr && !worldObj->multiplayerWorld && --m_timeUntilNextFeather <= 0)
    {
        if (rand.nextInt(100) < 15 && ChocoCraftItems::chocoboFeather != nullptr)
            dropItem(ChocoCraftItems::chocoboFeather->shiftedIndex, 1);
        m_timeUntilNextFeather = 600 + rand.nextInt(600);
    }

    // Straw + cauldron pen healing
    doStrawHealing();

    // Water buoyancy for water-crossing chocobos (Blue, White, Black, Gold, Pink, Red)
    if (canCrossWater() && isInWater())
    {
        if (motionY < 0.08)
            motionY += 0.06;
    }

    // ========================================================================
    // RIDER STEERING & FLIGHT (Xbox Controller P1/P2 + PC Keyboard)
    // ========================================================================
    EntityPlayer *rider = dynamic_cast<EntityPlayer *>(riddenByEntity);
    if (rider != nullptr)
    {
        // IMPORTANT: World::updateEntityWithOptionalForce calls updateRidden() instead of
        // EntityPlayerSP::onLivingUpdate() while riding, so movementInput MUST be refreshed here!
        float fwd = 0.0f;
        float strafe = 0.0f;
        bool jumpPressed = false;
        bool sneakPressed = false;

        if (EntityPlayerSP *sp = dynamic_cast<EntityPlayerSP *>(rider))
        {
            if (sp->movementInput != nullptr)
            {
                sp->movementInput->updatePlayerMoveState(sp);
                fwd = sp->movementInput->moveForward;
                strafe = sp->movementInput->moveStrafe;
                jumpPressed = sp->movementInput->jump;
                sneakPressed = sp->movementInput->sneak;
            }
        }
        else
        {
            fwd = rider->moveForward;
            strafe = rider->moveStrafing;
            jumpPressed = rider->isJumping;
            sneakPressed = rider->isSneaking();
        }

        // Prevent any fall damage to either rider or chocobo while mounted!
        fallDistance = 0.0f;
        rider->fallDistance = 0.0f;

        // Rider potion buffs (Blue=Water Breathing, Black=Night Vision, Purple=Fire Resistance)
        applyRiderBuffs(rider);

        // Dismount with Sneak (L3 / Shift):
        // - If flying high in air on a flying chocobo, Sneak descends smoothly like Creative flight!
        // - On ground, in water, or on non-flying chocobos, Sneak dismounts cleanly (after grace ticks).
        if (sneakPressed && m_mountGraceTicks <= 0)
        {
            if (canFly() && m_flying && !onGround && !isInWater())
            {
                // Descend smoothly while flying (like Creative flight / ChocoCraft EntityChocoboRideable)
                motionY = -0.34;
            }
            else
            {
                dismountChocobo(rider);
                EntityAnimal::onLivingUpdate();
                return;
            }
        }

        // Face rider's look direction
        rotationYaw = rider->rotationYaw;
        prevRotationYaw = rotationYaw;
        rotationYawHead = rotationYaw;
        renderYawOffset = rotationYaw;
        rotationPitch = 0.0f;

        // Compute horizontal speed based on medium (air / water / land)
        const bool inWaterNow = isInWater();
        float speed = getRideSpeed();
        if (inWaterNow)
            speed = getWaterRideSpeed();
        else if (!onGround && (m_flying || canFly()))
            speed = getAirRideSpeed();

        // When sneaking in air to descend, undo the 0.3x sneak throttle from MovementInput
        if (sneakPressed && canFly() && m_flying && !onGround)
        {
            fwd = std::clamp(fwd / 0.3f, -1.0f, 1.0f);
            strafe = std::clamp(strafe / 0.3f, -1.0f, 1.0f);
        }

        const float yawRad = rotationYaw * 3.14159265f / 180.0f;
        const float sinY = std::sin(yawRad);
        const float cosY = std::cos(yawRad);
        const double moveX = (-sinY * fwd + cosY * strafe) * static_cast<double>(speed);
        const double moveZ = (cosY * fwd + sinY * strafe) * static_cast<double>(speed);

        // Responsive direct steering with smooth damping (zero AI drift!)
        if (std::fabs(fwd) > 0.01f || std::fabs(strafe) > 0.01f)
        {
            motionX = motionX * 0.35 + moveX;
            motionZ = motionZ * 0.35 + moveZ;
        }
        else
        {
            motionX *= 0.45;
            motionZ *= 0.45;
            if (std::fabs(motionX) < 0.005) motionX = 0.0;
            if (std::fabs(motionZ) < 0.005) motionZ = 0.0;
        }

        // Vertical control: Jump / High-Jump / Glide / Creative-style Flight for Gold, Pink, Red, Purple!
        if (jumpPressed)
        {
            if (onGround)
            {
                motionY = static_cast<double>(getJumpVelocity());
                onGround = false;
                m_isHighJumping = canJumpHigh();
                if (canFly())
                    m_flying = true;
                if (worldObj != nullptr)
                    worldObj->playSoundAtEntity(this, getLivingSound(), 0.5f, 1.25f);
            }
            else if (canFly())
            {
                // Gold / Pink / Red / Purple Chocobo FLIGHT! (Hold A to ascend smoothly)
                m_flying = true;
                m_gliding = true;
                motionY = 0.44; // After EntityLiving's -0.08 gravity -> smooth +0.36 climb
                if (m_flyParticleCooldown <= 0 && worldObj != nullptr)
                {
                    m_flyParticleCooldown = 8;
                    worldObj->spawnParticle("reddust", posX, posY + 0.25, posZ, 1.0, 0.85, 0.1);
                }
            }
            else if (inWaterNow)
            {
                motionY = 0.22;
            }
        }
        else if (!onGround)
        {
            if (canFly() && m_flying && !sneakPressed)
            {
                // Gentle hover-glide when releasing A in midair
                m_gliding = true;
                if (motionY < -0.06)
                    motionY = -0.06; // After -0.08 gravity -> gentle -0.14 sink
            }
            else if (!sneakPressed && motionY < 0.0)
            {
                // All chocobos glide gently with wings spread when falling!
                m_gliding = true;
                motionY *= 0.72;
                if (motionY < -0.14)
                    motionY = -0.14;
            }
        }

        // Zero AI fields before EntityLiving physics tick
        moveForward = 0.0f;
        moveStrafing = 0.0f;
        isJumping = false;

        EntityAnimal::onLivingUpdate();

        // Keep fallDistance cleared post-physics as well (double guarantee!)
        fallDistance = 0.0f;
        if (riddenByEntity != nullptr)
            riddenByEntity->fallDistance = 0.0f;
        return;
    }

    // Unmounted gentle wing-glide on fall
    if (!onGround && motionY < 0.0)
    {
        m_gliding = true;
        motionY *= 0.8;
    }

    EntityAnimal::onLivingUpdate();
}

bool EntityChocobo::isWheat(ItemStack *itemstack) const
{
    if (itemstack == nullptr)
        return false;
    const int id = itemstack->itemID;
    return (ChocoCraftItems::gysahlLoverly != nullptr && id == ChocoCraftItems::gysahlLoverly->shiftedIndex) ||
           (ChocoCraftItems::gysahlGolden  != nullptr && id == ChocoCraftItems::gysahlGolden->shiftedIndex) ||
           (ChocoCraftItems::gysahlGreens  != nullptr && id == ChocoCraftItems::gysahlGreens->shiftedIndex);
}

// ChocoCraft 4.1.5 exact genetics (colours/*.java getBabyAnimalColor)
ChocoboColor EntityChocobo::breedColor(ChocoboColor a, ChocoboColor b, bool bothFedGold, Random &rng)
{
    const int roll = rng.nextInt(100);

    // Purple + Yellow..Black -> other color; Purple + Gold/Pink/Red/Purple -> Purple
    if (a == ChocoboColor::Purple || b == ChocoboColor::Purple)
    {
        ChocoboColor other = (a == ChocoboColor::Purple) ? b : a;
        if (other == ChocoboColor::Yellow || other == ChocoboColor::Green ||
            other == ChocoboColor::Blue   || other == ChocoboColor::White ||
            other == ChocoboColor::Black)
            return other;
        return ChocoboColor::Purple;
    }

    // Normalize order
    if (static_cast<int>(a) > static_cast<int>(b))
        std::swap(a, b);

    // Gold/Pink/Red + Gold/Pink/Red -> Gold (20% if both fed Golden Gysahl), else Yellow
    if ((a == ChocoboColor::Gold || a == ChocoboColor::Pink || a == ChocoboColor::Red) &&
        (b == ChocoboColor::Gold || b == ChocoboColor::Pink || b == ChocoboColor::Red))
    {
        return (bothFedGold && roll <= 20) ? ChocoboColor::Gold : ChocoboColor::Yellow;
    }

    // White + Black -> Gold (50% if both fed Golden Gysahl; 25% if Loverly/normal Gysahl!)
    if (a == ChocoboColor::White && b == ChocoboColor::Black)
    {
        if (bothFedGold)
        {
            if (roll > 50) return ChocoboColor::Gold;
            if (roll > 25) return ChocoboColor::Black;
            return ChocoboColor::White;
        }
        if (roll > 75) return ChocoboColor::Gold;
        if (roll > 40) return ChocoboColor::Black;
        return ChocoboColor::White;
    }

    // Gold/Pink/Red + White/Black -> small chance Gold when fed gold, else partner
    if (b == ChocoboColor::Gold || b == ChocoboColor::Pink || b == ChocoboColor::Red)
    {
        if (bothFedGold && a == ChocoboColor::White && roll > 70) return ChocoboColor::Gold;
        if (bothFedGold && a == ChocoboColor::Black && roll > 90) return ChocoboColor::Gold;
        return a;
    }

    // Yellow + Yellow -> Yellow, Green, Blue (ChocoCraft 4.1.5 EntityChocoboYellow.java)
    if (a == ChocoboColor::Yellow && b == ChocoboColor::Yellow)
    {
        if (bothFedGold)
        {
            if (roll > 80) return ChocoboColor::Blue;
            if (roll > 60) return ChocoboColor::Green;
            return ChocoboColor::Yellow;
        }
        if (roll > 60) return ChocoboColor::Blue;
        if (roll > 20) return ChocoboColor::Green;
        return ChocoboColor::Yellow;
    }

    // Green + Blue -> White, Blue, Green
    if (a == ChocoboColor::Green && b == ChocoboColor::Blue)
    {
        if (bothFedGold)
        {
            if (roll > 60) return ChocoboColor::White;
            if (roll > 30) return ChocoboColor::Blue;
            return ChocoboColor::Green;
        }
        if (roll > 80) return ChocoboColor::White;
        if (roll > 40) return ChocoboColor::Blue;
        return ChocoboColor::Green;
    }

    // Yellow + White -> Black, White, Yellow
    if (a == ChocoboColor::Yellow && b == ChocoboColor::White)
    {
        if (bothFedGold)
        {
            if (roll > 60) return ChocoboColor::Black;
            if (roll > 30) return ChocoboColor::White;
            return ChocoboColor::Yellow;
        }
        if (roll > 80) return ChocoboColor::Black;
        if (roll > 40) return ChocoboColor::White;
        return ChocoboColor::Yellow;
    }

    // Same color
    if (a == b)
        return a;

    return (roll < 50) ? a : b;
}

EntityAnimal *EntityChocobo::spawnBabyAnimal(EntityAnimal *mate)
{
    EntityChocobo *baby = new EntityChocobo(worldObj);
    EntityChocobo *other = dynamic_cast<EntityChocobo *>(mate);
    const bool bothGold = m_fedGold && (other != nullptr && other->m_fedGold);
    ChocoboColor childColor = other ? breedColor(m_color, other->getColor(), bothGold, rand) : m_color;
    baby->setColor(childColor);
    baby->setTamed(m_tamed || (other != nullptr && other->isTamed()));
    baby->setOwnerName(!m_ownerName.empty() ? m_ownerName : (other ? other->getOwnerName() : ""));
    baby->setSize(0.5f, 0.5f);
    baby->m_wasChild = true;
    m_fedGold = false;
    if (other != nullptr)
        other->m_fedGold = false;
    return baby;
}

bool EntityChocobo::interact(EntityPlayer *player)
{
    if (player == nullptr || worldObj == nullptr)
        return false;

    ItemStack *held = player->inventory ? player->inventory->getCurrentItem() : nullptr;

    // 1. Gysahl Greens: tame wild chocobo, heal tamed chocobo, or accelerate baby growth
    if (held != nullptr && ChocoCraftItems::gysahlGreens != nullptr &&
        held->itemID == ChocoCraftItems::gysahlGreens->shiftedIndex)
    {
        if (!m_tamed)
        {
            consumeHeldItem(player);
            if (player->capabilities.isCreativeMode || rand.nextInt(3) == 0)
            {
                m_tamed = true;
                m_ownerName = player->username;
                m_aiMode = ChocoboAiMode::Wander;
                setHealth(getMaxHealth());
                spawnParticles("heart", 10);
                worldObj->playSoundAtEntity(this, getLivingSound(), 0.9f, 1.2f);
            }
            else
            {
                spawnParticles("smoke", 7);
            }
            return true;
        }
        if (isChild())
        {
            consumeHeldItem(player);
            setGrowingAge(getGrowingAge() / 2);
            spawnParticles("heart", 6);
            return true;
        }
        if (getHealth() < getMaxHealth())
        {
            consumeHeldItem(player);
            heal(10);
            spawnParticles("heart", 6);
            return true;
        }
        // Full-health tamed adult -> enter love mode (also works with Gysahl Greens for convenience!)
        m_fedGold = false;
        return EntityAnimal::interact(player);
    }

    // 2. Loverly Gysahl / Golden Gysahl -> breed mode (ChocoCraft 4.1.5)
    const bool isLoverly = held != nullptr && ChocoCraftItems::gysahlLoverly != nullptr &&
                           held->itemID == ChocoCraftItems::gysahlLoverly->shiftedIndex;
    const bool isGolden = held != nullptr && ChocoCraftItems::gysahlGolden != nullptr &&
                          held->itemID == ChocoCraftItems::gysahlGolden->shiftedIndex;
    if ((isLoverly || isGolden) && m_tamed && !isChild())
    {
        m_fedGold = isGolden;
        return EntityAnimal::interact(player);
    }

    // 3. Gysahl Cake -> grow Chicobo immediately into adult! (EntityChicobo.java:253)
    if (held != nullptr && ChocoCraftItems::gysahlCake != nullptr &&
        held->itemID == ChocoCraftItems::gysahlCake->shiftedIndex)
    {
        if (m_tamed && isChild())
        {
            consumeHeldItem(player);
            growUpToAdult();
            return true;
        }
        spawnParticles("smoke", 5);
        return true;
    }

    // 4. Pink / Red Gysahl -> paint Gold Chocobo into Pink or Red! (EntityChocoboGold.java:121)
    const bool isPink = held != nullptr && ChocoCraftItems::gysahlPink != nullptr &&
                        held->itemID == ChocoCraftItems::gysahlPink->shiftedIndex;
    const bool isRed  = held != nullptr && ChocoCraftItems::gysahlRed != nullptr &&
                        held->itemID == ChocoCraftItems::gysahlRed->shiftedIndex;
    if (isPink || isRed)
    {
        if (m_tamed && !isChild() && m_color == ChocoboColor::Gold)
        {
            consumeHeldItem(player);
            setColor(isPink ? ChocoboColor::Pink : ChocoboColor::Red);
            spawnParticles("explode", 16);
            worldObj->playSoundAtEntity(this, getLivingSound(), 0.9f, 1.15f);
        }
        else
        {
            spawnParticles("smoke", 6);
        }
        return true;
    }

    // 5. Chocobo Feather -> cycle Follow / Stay / Wander (ChocoCraft 4.1.5)
    if (held != nullptr && ChocoCraftItems::chocoboFeather != nullptr &&
        held->itemID == ChocoCraftItems::chocoboFeather->shiftedIndex)
    {
        if (m_tamed)
        {
            if (m_aiMode == ChocoboAiMode::Follow)
                m_aiMode = ChocoboAiMode::Stay;
            else if (m_aiMode == ChocoboAiMode::Stay && !m_saddled && !m_packbags)
                m_aiMode = ChocoboAiMode::Wander;
            else
                m_aiMode = ChocoboAiMode::Follow;
            spawnParticles(m_aiMode == ChocoboAiMode::Follow ? "heart" : "smoke", 6);
        }
        return true;
    }

    // 6. Equip Chocobo Saddle (or vanilla Saddle) on tamed adult without pack bags
    const bool isSaddleItem = held != nullptr && (
        (ChocoCraftItems::chocoboSaddle != nullptr && held->itemID == ChocoCraftItems::chocoboSaddle->shiftedIndex) ||
        (Item::saddle != nullptr && held->itemID == Item::saddle->shiftedIndex));
    if (isSaddleItem)
    {
        if (m_tamed && !isChild() && !m_saddled && !m_packbags)
        {
            setSaddled(true);
            m_aiMode = ChocoboAiMode::Stay; // Saddled chocobo stays put until ridden or told to follow!
            consumeHeldItem(player);
            worldObj->playSoundAtEntity(this, "random.pop", 0.8f, 1.0f);
            spawnParticles("heart", 5);
        }
        else
        {
            spawnParticles("smoke", 5);
        }
        return true;
    }

    // 7. Equip Chocobo Saddlebags (27 slots) on tamed SADDLED adult
    if (held != nullptr && ChocoCraftItems::chocoboSaddlebags != nullptr &&
        held->itemID == ChocoCraftItems::chocoboSaddlebags->shiftedIndex)
    {
        if (m_tamed && !isChild() && m_saddled && !m_saddlebags && !m_packbags)
        {
            setSaddlebags(true);
            consumeHeldItem(player);
            worldObj->playSoundAtEntity(this, "random.pop", 0.8f, 0.9f);
            spawnParticles("heart", 5);
        }
        else
        {
            spawnParticles("smoke", 5);
        }
        return true;
    }

    // 8. Equip Chocobo Pack Bags (54 slots) on tamed UNSADDLED adult
    if (held != nullptr && ChocoCraftItems::chocoboPackbags != nullptr &&
        held->itemID == ChocoCraftItems::chocoboPackbags->shiftedIndex)
    {
        if (m_tamed && !isChild() && !m_saddled && !m_packbags)
        {
            setPackbags(true);
            m_aiMode = ChocoboAiMode::Follow;
            consumeHeldItem(player);
            worldObj->playSoundAtEntity(this, "random.pop", 0.8f, 0.85f);
            spawnParticles("heart", 5);
        }
        else
        {
            spawnParticles("smoke", 5);
        }
        return true;
    }

    // 9. Sneak + Interact on tamed chocobo -> Open Chocobo Menu (Rename, Bags, AI Mode, Mount)
    if (m_tamed && player->isSneaking())
    {
        Minecraft *mc = Minecraft::getMinecraft();
        if (mc != nullptr)
        {
            const bool isP2 = (player == mc->thePlayer2);
            if (isP2 && mc->isSplitScreenActive())
                mc->displayPlayerScreen(1, new GuiChocoboMenu(this, player, 1));
            else
                mc->displayGuiScreen(new GuiChocoboMenu(this, player, 0));
            return true;
        }
    }

    // 10. MOUNT: ONLY if tamed, SADDLED, adult, and NOT packbagged!
    if (m_tamed && m_saddled && !m_packbags && !isChild() && (riddenByEntity == nullptr || riddenByEntity == player))
    {
        mountChocobo(player);
        return true;
    }

    // Without saddle: NEVER mount! Emit smoke cue so the player knows a saddle is required.
    if (!m_saddled && !isChild())
    {
        spawnParticles("smoke", 5);
        return true;
    }

    return EntityAnimal::interact(player);
}

int_t EntityChocobo::getDropItemId()
{
    if (isBurning() && m_color != ChocoboColor::Purple && ChocoCraftItems::chocoboLegCooked != nullptr)
        return ChocoCraftItems::chocoboLegCooked->shiftedIndex;
    if (ChocoCraftItems::chocoboLegRaw != nullptr)
        return ChocoCraftItems::chocoboLegRaw->shiftedIndex;
    return ChocoCraftItems::chocoboFeather ? ChocoCraftItems::chocoboFeather->shiftedIndex : Item::feather->shiftedIndex;
}

void EntityChocobo::dropAllGearAndBags()
{
    if (m_gearDropped || worldObj == nullptr || worldObj->multiplayerWorld)
        return;
    m_gearDropped = true;

    if (m_saddled && ChocoCraftItems::chocoboSaddle != nullptr)
        dropItem(ChocoCraftItems::chocoboSaddle->shiftedIndex, 1);
    if (m_saddlebags)
    {
        if (ChocoCraftItems::chocoboSaddlebags != nullptr)
            dropItem(ChocoCraftItems::chocoboSaddlebags->shiftedIndex, 1);
        m_inventory.dropAllContents(worldObj, posX, posY, posZ);
    }
    if (m_packbags)
    {
        if (ChocoCraftItems::chocoboPackbags != nullptr)
            dropItem(ChocoCraftItems::chocoboPackbags->shiftedIndex, 1);
        m_inventory.dropAllContents(worldObj, posX, posY, posZ);
    }
    // Drop 1..3 Chocobo Feathers on death (EntityChocobo.java:374)
    if (ChocoCraftItems::chocoboFeather != nullptr)
    {
        const int feathers = 1 + rand.nextInt(3);
        for (int i = 0; i < feathers; ++i)
            dropItem(ChocoCraftItems::chocoboFeather->shiftedIndex, 1);
    }
}

void EntityChocobo::onDeath(Entity *entity)
{
    dropAllGearAndBags();
    EntityAnimal::onDeath(entity);
}

void EntityChocobo::onDeath(const DamageSource &source)
{
    dropAllGearAndBags();
    EntityAnimal::onDeath(source);
}

void EntityChocobo::writeEntityToNBT(NBTTagCompound *nbt)
{
    EntityAnimal::writeEntityToNBT(nbt);
    nbt->setInteger("ChocoboColor", static_cast<int_t>(m_color));
    nbt->setBoolean("IsMale", m_isMale);
    nbt->setBoolean("Tamed", m_tamed);
    nbt->setBoolean("Saddled", m_saddled);
    nbt->setBoolean("Saddlebags", m_saddlebags);
    nbt->setBoolean("Packbags", m_packbags);
    nbt->setBoolean("Hidename", m_hidename);
    nbt->setInteger("AiMode", static_cast<int_t>(m_aiMode));
    nbt->setString("Owner", m_ownerName);
    nbt->setString("CustomName", m_customName);
    m_inventory.writeToNBT(nbt);
}

void EntityChocobo::readEntityFromNBT(NBTTagCompound *nbt)
{
    EntityAnimal::readEntityFromNBT(nbt);
    int_t c = nbt->getInteger("ChocoboColor");
    setColor(static_cast<ChocoboColor>(std::clamp<int_t>(c, 0, 8)));
    m_isMale = nbt->hasKey("IsMale") ? nbt->getBoolean("IsMale") : true;
    m_tamed = nbt->getBoolean("Tamed");
    setSaddled(nbt->getBoolean("Saddled"));
    setSaddlebags(nbt->getBoolean("Saddlebags"));
    if (nbt->getBoolean("Packbags"))
        setPackbags(true);
    m_hidename = nbt->getBoolean("Hidename");
    int_t ai = nbt->getInteger("AiMode");
    m_aiMode = static_cast<ChocoboAiMode>(std::clamp<int_t>(ai, 0, 2));
    m_ownerName = nbt->getString("Owner");
    std::string cname = nbt->getString("CustomName");
    if (!cname.empty())
        setCustomName(cname);
    m_inventory.readFromNBT(nbt);
    m_wasChild = isChild();
    setSize(m_wasChild ? 0.5f : 0.95f, m_wasChild ? 0.5f : 1.85f);
}
