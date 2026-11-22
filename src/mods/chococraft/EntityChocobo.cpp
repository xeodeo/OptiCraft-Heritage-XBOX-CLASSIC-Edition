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
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/StringTranslate.h"

#include <cmath>
#include <algorithm>

// ============================================================================
// InventoryChocoboSaddlebag (27-slot persistent inventory)
// ============================================================================

InventoryChocoboSaddlebag::InventoryChocoboSaddlebag(const std::string &title)
    : m_title(title)
{
    for (int_t i = 0; i < SLOT_COUNT; ++i)
        m_slots[i] = nullptr;
}

InventoryChocoboSaddlebag::~InventoryChocoboSaddlebag()
{
    for (int_t i = 0; i < SLOT_COUNT; ++i)
    {
        delete m_slots[i];
        m_slots[i] = nullptr;
    }
}

ItemStack *InventoryChocoboSaddlebag::getStackInSlot(int_t slot)
{
    if (slot < 0 || slot >= SLOT_COUNT)
        return nullptr;
    return m_slots[slot];
}

ItemStack *InventoryChocoboSaddlebag::decrStackSize(int_t slot, int_t amount)
{
    if (slot < 0 || slot >= SLOT_COUNT || m_slots[slot] == nullptr)
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
    return nullptr; // Persistent saddlebag: never drop items when closing GUI
}

void InventoryChocoboSaddlebag::setInventorySlotContents(int_t slot, ItemStack *stack)
{
    if (slot < 0 || slot >= SLOT_COUNT)
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
    for (int_t i = 0; i < SLOT_COUNT; ++i)
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
    for (int_t i = 0; i < SLOT_COUNT; ++i)
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
        if (slot >= 0 && slot < SLOT_COUNT)
            m_slots[slot] = ItemStack::loadItemStackFromNBT(slotTag);
    }
}

void InventoryChocoboSaddlebag::dropAllContents(World *world, double x, double y, double z)
{
    if (world == nullptr || world->multiplayerWorld)
        return;
    for (int_t i = 0; i < SLOT_COUNT; ++i)
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
// EntityChocobo
// ============================================================================

EntityChocobo::EntityChocobo(World *world)
    : EntityAnimal(world)
    , m_color(ChocoboColor::Yellow)
    , m_tamed(false)
    , m_saddled(false)
    , m_saddlebags(false)
    , m_aiMode(ChocoboAiMode::Wander)
    , m_inventory("Chocobo Saddlebags")
    , m_wingFlap(0.0f)
    , m_prevWingFlap(0.0f)
    , m_wingFlapSpeed(0.0f)
    , m_flyCooldown(0)
{
    ensureEntityInit();
    texture = "/mob/chicken.png";
    setSize(0.95f, 1.85f);
    moveSpeed = 0.65f;
    stepHeight = 1.0f;
    setHealth(getMaxHealth());
}

void EntityChocobo::setColor(ChocoboColor color)
{
    m_color = color;
    if (m_color == ChocoboColor::Green || m_color == ChocoboColor::Gold)
        stepHeight = 1.6f; // Mountain climber!
    else if (m_color == ChocoboColor::Black)
        stepHeight = 1.15f;
    else
        stepHeight = 1.0f;
    if (getHealth() > getMaxHealth())
        setHealth(getMaxHealth());
}

void EntityChocobo::setCustomName(const std::string &name)
{
    m_customName = name;
    if (!m_customName.empty())
        m_inventory.setTitle(m_customName);
    else
        m_inventory.setTitle("Chocobo Saddlebags");
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
    default:                   return "Yellow";
    }
}

const char *EntityChocobo::getColorAbility(ChocoboColor color, bool spanish)
{
    switch (color)
    {
    case ChocoboColor::Yellow: return spanish ? "Corredor clasico" : "Classic runner";
    case ChocoboColor::Green:  return spanish ? "Escala montanas" : "Mountain climber";
    case ChocoboColor::Blue:   return spanish ? "Nada rapido en agua" : "Fast water swimmer";
    case ChocoboColor::White:  return spanish ? "Agil y gran salto" : "Agile high jumper";
    case ChocoboColor::Black:  return spanish ? "Veloz, sin dano caida" : "Fast, no fall dmg";
    case ChocoboColor::Gold:   return spanish ? "Vuela (pulsa A), nada y escala" : "Flies (hold A), swims & climbs";
    default:                   return "";
    }
}

int_t EntityChocobo::getMaxHealth() const
{
    switch (m_color)
    {
    case ChocoboColor::Gold:  return 40;
    case ChocoboColor::Black: return 32;
    case ChocoboColor::White: return 30;
    case ChocoboColor::Green:
    case ChocoboColor::Blue:  return 26;
    default:                  return 22;
    }
}

float EntityChocobo::getRideSpeed() const
{
    switch (m_color)
    {
    case ChocoboColor::Gold:  return 0.38f;
    case ChocoboColor::Black: return 0.37f;
    case ChocoboColor::White: return 0.33f;
    case ChocoboColor::Green: return 0.29f;
    case ChocoboColor::Blue:  return 0.28f;
    default:                  return 0.26f;
    }
}

float EntityChocobo::getJumpVelocity() const
{
    switch (m_color)
    {
    case ChocoboColor::Gold:  return 0.72f;
    case ChocoboColor::Black: return 0.68f;
    case ChocoboColor::White: return 0.65f;
    case ChocoboColor::Green: return 0.62f;
    default:                  return 0.54f;
    }
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

void EntityChocobo::updateRiderPosition()
{
    if (riddenByEntity == nullptr)
        return;
    const double seatY = isChild() ? 0.65 : getMountedYOffset();
    riddenByEntity->setPosition(posX, posY + seatY + riddenByEntity->getYOffset(), posZ);
}

void EntityChocobo::fall(float dist)
{
    if (m_color == ChocoboColor::Gold || m_color == ChocoboColor::Black)
    {
        fallDistance = 0.0f;
        return;
    }
    if (m_color == ChocoboColor::White || m_color == ChocoboColor::Green)
        dist *= 0.45f;
    else
        dist *= 0.65f;
    if (dist > 3.5f)
        EntityAnimal::fall(dist);
}

void EntityChocobo::onLivingUpdate()
{
    m_prevWingFlap = m_wingFlap;
    if (m_flyCooldown > 0)
        m_flyCooldown--;

    // Gentle gliding & wing flap animation
    if (!onGround)
    {
        m_wingFlapSpeed = std::min(1.0f, m_wingFlapSpeed + 0.18f);
        m_wingFlap += 0.55f;
        const double maxFall = (m_color == ChocoboColor::Gold) ? -0.11 : -0.22;
        if (motionY < maxFall)
            motionY = maxFall;
    }
    else
    {
        m_wingFlapSpeed *= 0.65f;
        m_wingFlap *= 0.6f;
    }

    // Blue & Gold chocobos float and swim smoothly on water!
    if ((m_color == ChocoboColor::Blue || m_color == ChocoboColor::Gold) && isInWater())
    {
        if (motionY < 0.08)
            motionY += 0.09;
    }

    // Rider steering with full Xbox controller support (P1 and P2!)
    EntityPlayer *rider = dynamic_cast<EntityPlayer *>(riddenByEntity);
    if (rider != nullptr)
    {
        // Sneak (L3) dismounts cleanly
        if (rider->isSneaking())
        {
            rider->mountEntity(nullptr);
            EntityAnimal::onLivingUpdate();
            return;
        }

        // Read rider movement & jump directly from EntityPlayer
        float fwd = rider->moveForward;
        float strafe = rider->moveStrafing;
        bool jumpPressed = rider->isJumping;

        // Also check EntityPlayerSP::movementInput in case moveForward wasn't copied while ridden
        if (EntityPlayerSP *sp = dynamic_cast<EntityPlayerSP *>(rider))
        {
            if (sp->movementInput != nullptr)
            {
                fwd = sp->movementInput->moveForward;
                strafe = sp->movementInput->moveStrafe;
                jumpPressed = sp->movementInput->jump;
            }
        }

        rotationYaw = rider->rotationYaw;
        prevRotationYaw = rotationYaw;
        rotationYawHead = rotationYaw;
        renderYawOffset = rotationYaw;
        rotationPitch = rider->rotationPitch * 0.4f;

        const float speed = getRideSpeed();
        const float yawRad = rotationYaw * 3.14159265f / 180.0f;
        const float sinY = std::sin(yawRad);
        const float cosY = std::cos(yawRad);

        double moveX = (-sinY * fwd + cosY * strafe) * speed;
        double moveZ = (cosY * fwd + sinY * strafe) * speed;

        if ((m_color == ChocoboColor::Blue || m_color == ChocoboColor::Gold) && isInWater())
        {
            moveX *= 1.35;
            moveZ *= 1.35;
        }

        motionX = motionX * 0.45 + moveX;
        motionZ = motionZ * 0.45 + moveZ;

        // Jump on ground, or FLY if Gold Chocobo!
        if (jumpPressed)
        {
            if (onGround)
            {
                motionY = getJumpVelocity();
                onGround = false;
                m_flyCooldown = 4;
                if (worldObj != nullptr)
                    worldObj->playSoundAtEntity(this, "mob.chicken", 0.5f, 1.3f);
            }
            else if (m_color == ChocoboColor::Gold)
            {
                // Smooth responsive golden chocobo flight while holding A
                motionY = std::min(0.42, motionY + 0.14);
                fallDistance = 0.0f;
                if (m_flyCooldown <= 0)
                {
                    m_flyCooldown = 10;
                    if (worldObj != nullptr)
                        worldObj->spawnParticle("reddust", posX, posY + 0.3, posZ, 1.0, 0.85, 0.1);
                }
            }
        }

        // Prevent wild AI from overriding rider motion
        moveForward = 0.0f;
        moveStrafing = 0.0f;
        isJumping = false;
        EntityAnimal::onLivingUpdate();
        return;
    }

    // Tamed Follow / Stay behavior when not ridden
    if (m_tamed && worldObj != nullptr)
    {
        if (m_aiMode == ChocoboAiMode::Stay)
        {
            moveForward = 0.0f;
            moveStrafing = 0.0f;
            motionX *= 0.2;
            motionZ *= 0.2;
        }
        else if (m_aiMode == ChocoboAiMode::Follow)
        {
            EntityPlayer *owner = worldObj->getClosestPlayerToEntity(this, 24.0);
            if (owner != nullptr && (m_ownerName.empty() || std::string(owner->username) == m_ownerName))
            {
                const double distSq = getDistanceSqToEntity(owner);
                if (distSq > 16.0 && distSq < 576.0)
                {
                    const double dx = owner->posX - posX;
                    const double dz = owner->posZ - posZ;
                    const double dist = std::sqrt(dx * dx + dz * dz);
                    if (dist > 0.1)
                    {
                        rotationYaw = static_cast<float>(std::atan2(dz, dx) * 180.0 / 3.14159265) - 90.0f;
                        rotationYawHead = rotationYaw;
                        motionX = (dx / dist) * 0.22;
                        motionZ = (dz / dist) * 0.22;
                        if (isCollidedHorizontally && onGround)
                            motionY = 0.45;
                    }
                }
            }
        }
    }

    EntityAnimal::onLivingUpdate();
}

bool EntityChocobo::isWheat(ItemStack *itemstack) const
{
    if (itemstack == nullptr)
        return false;
    return ChocoCraftItems::gysahlGreens != nullptr &&
           itemstack->itemID == ChocoCraftItems::gysahlGreens->shiftedIndex;
}

ChocoboColor EntityChocobo::breedColor(ChocoboColor a, ChocoboColor b, Random &rng)
{
    // Normalize order so (a, b) == (b, a)
    if (static_cast<int>(a) > static_cast<int>(b))
        std::swap(a, b);

    const int roll = rng.nextInt(100);
    // White + Black -> Gold (50%)!
    if (a == ChocoboColor::White && b == ChocoboColor::Black)
    {
        if (roll < 50) return ChocoboColor::Gold;
        return (roll < 75) ? ChocoboColor::White : ChocoboColor::Black;
    }
    // Gold + anything -> Gold (55%) or partner
    if (b == ChocoboColor::Gold)
    {
        return (roll < 55) ? ChocoboColor::Gold : a;
    }
    // Yellow + Yellow -> mostly Yellow, chance of Green (mountain) or Blue (river)
    if (a == ChocoboColor::Yellow && b == ChocoboColor::Yellow)
    {
        if (roll < 64) return ChocoboColor::Yellow;
        return (roll < 82) ? ChocoboColor::Green : ChocoboColor::Blue;
    }
    // Green + Blue -> White (45%), Black (45%), Yellow (10%)
    if (a == ChocoboColor::Green && b == ChocoboColor::Blue)
    {
        if (roll < 45) return ChocoboColor::White;
        if (roll < 90) return ChocoboColor::Black;
        return ChocoboColor::Yellow;
    }
    // Yellow + Green -> Green (60%), White (25%), Yellow (15%)
    if (a == ChocoboColor::Yellow && b == ChocoboColor::Green)
    {
        if (roll < 60) return ChocoboColor::Green;
        if (roll < 85) return ChocoboColor::White;
        return ChocoboColor::Yellow;
    }
    // Yellow + Blue -> Blue (60%), Black (25%), Yellow (15%)
    if (a == ChocoboColor::Yellow && b == ChocoboColor::Blue)
    {
        if (roll < 60) return ChocoboColor::Blue;
        if (roll < 85) return ChocoboColor::Black;
        return ChocoboColor::Yellow;
    }
    // Same special color
    if (a == b)
        return a;
    return (roll < 50) ? a : b;
}

EntityAnimal *EntityChocobo::spawnBabyAnimal(EntityAnimal *mate)
{
    EntityChocobo *baby = new EntityChocobo(worldObj);
    EntityChocobo *other = dynamic_cast<EntityChocobo *>(mate);
    ChocoboColor childColor = other ? breedColor(m_color, other->getColor(), rand) : m_color;
    baby->setColor(childColor);
    baby->setTamed(m_tamed || (other && other->isTamed()));
    baby->setOwnerName(!m_ownerName.empty() ? m_ownerName : (other ? other->getOwnerName() : ""));
    return baby;
}

bool EntityChocobo::interact(EntityPlayer *player)
{
    if (player == nullptr || worldObj == nullptr)
        return false;

    ItemStack *held = player->inventory ? player->inventory->getCurrentItem() : nullptr;

    // 1. Gysahl Greens: tame wild chocobo, heal tamed chocobo, speed baby growth, or breed!
    if (held != nullptr && isWheat(held))
    {
        if (!m_tamed)
        {
            if (!player->capabilities.isCreativeMode)
            {
                held->stackSize--;
                if (held->stackSize <= 0)
                    player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
            }
            m_tamed = true;
            m_ownerName = player->username;
            setHealth(getMaxHealth());
            spawnParticles("heart", 10);
            worldObj->playSoundAtEntity(this, "mob.chicken", 0.9f, 1.25f);
            return true;
        }
        if (isChild())
        {
            if (!player->capabilities.isCreativeMode)
            {
                held->stackSize--;
                if (held->stackSize <= 0)
                    player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
            }
            setGrowingAge(getGrowingAge() / 2);
            spawnParticles("heart", 6);
            return true;
        }
        if (getHealth() < getMaxHealth())
        {
            if (!player->capabilities.isCreativeMode)
            {
                held->stackSize--;
                if (held->stackSize <= 0)
                    player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
            }
            heal(6);
            spawnParticles("heart", 6);
            return true;
        }
        // Full-health tamed adult -> enter love mode for breeding!
        return EntityAnimal::interact(player);
    }

    // 2. Equip Chocobo Saddle (or vanilla Saddle) on tamed adult
    const bool isSaddleItem = held != nullptr && (
        (ChocoCraftItems::chocoboSaddle != nullptr && held->itemID == ChocoCraftItems::chocoboSaddle->shiftedIndex) ||
        (Item::saddle != nullptr && held->itemID == Item::saddle->shiftedIndex));
    if (isSaddleItem && m_tamed && !isChild() && !m_saddled)
    {
        m_saddled = true;
        if (!player->capabilities.isCreativeMode)
        {
            held->stackSize--;
            if (held->stackSize <= 0)
                player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
        }
        worldObj->playSoundAtEntity(this, "random.pop", 0.8f, 1.0f);
        spawnParticles("smoke", 6);
        return true;
    }

    // 3. Equip Chocobo Saddlebags on tamed adult
    const bool isBagsItem = held != nullptr &&
        ChocoCraftItems::chocoboSaddlebags != nullptr &&
        held->itemID == ChocoCraftItems::chocoboSaddlebags->shiftedIndex;
    if (isBagsItem && m_tamed && !isChild() && !m_saddlebags)
    {
        m_saddlebags = true;
        if (!player->capabilities.isCreativeMode)
        {
            held->stackSize--;
            if (held->stackSize <= 0)
                player->inventory->setInventorySlotContents(player->inventory->currentItem, nullptr);
        }
        worldObj->playSoundAtEntity(this, "random.pop", 0.8f, 0.9f);
        spawnParticles("smoke", 6);
        return true;
    }

    // 4. Sneak + Interact on tamed chocobo -> Open Chocobo Menu (Rename, Saddlebags, AI mode!)
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

    // 5. Mount tamed saddled adult chocobo!
    if (m_tamed && m_saddled && !isChild() && (riddenByEntity == nullptr || riddenByEntity == player))
    {
        player->mountEntity(this);
        return true;
    }

    // 6. Fallback: if tamed without saddle yet, open Chocobo Menu so player can name/check stats!
    if (m_tamed)
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

    return EntityAnimal::interact(player);
}

int_t EntityChocobo::getDropItemId()
{
    return ChocoCraftItems::chocoboFeather ? ChocoCraftItems::chocoboFeather->shiftedIndex : Item::feather->shiftedIndex;
}

void EntityChocobo::onDeath(Entity *entity)
{
    if (m_saddlebags)
        m_inventory.dropAllContents(worldObj, posX, posY, posZ);
    EntityAnimal::onDeath(entity);
}

void EntityChocobo::onDeath(const DamageSource &source)
{
    if (m_saddlebags)
        m_inventory.dropAllContents(worldObj, posX, posY, posZ);
    EntityAnimal::onDeath(source);
}

void EntityChocobo::writeEntityToNBT(NBTTagCompound *nbt)
{
    EntityAnimal::writeEntityToNBT(nbt);
    nbt->setInteger("ChocoboColor", static_cast<int_t>(m_color));
    nbt->setBoolean("Tamed", m_tamed);
    nbt->setBoolean("Saddled", m_saddled);
    nbt->setBoolean("Saddlebags", m_saddlebags);
    nbt->setInteger("AiMode", static_cast<int_t>(m_aiMode));
    nbt->setString("Owner", m_ownerName);
    nbt->setString("CustomName", m_customName);
    m_inventory.writeToNBT(nbt);
}

void EntityChocobo::readEntityFromNBT(NBTTagCompound *nbt)
{
    EntityAnimal::readEntityFromNBT(nbt);
    int_t c = nbt->getInteger("ChocoboColor");
    setColor(static_cast<ChocoboColor>(std::clamp<int_t>(c, 0, 5)));
    m_tamed = nbt->getBoolean("Tamed");
    m_saddled = nbt->getBoolean("Saddled");
    m_saddlebags = nbt->getBoolean("Saddlebags");
    int_t ai = nbt->getInteger("AiMode");
    m_aiMode = static_cast<ChocoboAiMode>(std::clamp<int_t>(ai, 0, 2));
    m_ownerName = nbt->getString("Owner");
    setCustomName(nbt->getString("CustomName"));
    m_inventory.readFromNBT(nbt);
}
