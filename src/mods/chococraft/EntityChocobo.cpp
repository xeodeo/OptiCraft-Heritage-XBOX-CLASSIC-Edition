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
#include "net/minecraft/src/MovementInputFromOptions.h"
#include "net/minecraft/src/EntityItem.h"
#include "net/minecraft/src/InventoryPlayer.h"
#include "net/minecraft/src/NBTTagCompound.h"
#include "net/minecraft/src/NBTTagList.h"
#include "net/minecraft/src/MathHelper.h"
#include "net/minecraft/src/Potion.h"
#include "net/minecraft/src/PotionEffect.h"
#include "net/minecraft/src/AxisAlignedBB.h"
#include "net/minecraft/src/GuiIngame.h"
#include "net/minecraft/src/StringTranslate.h"
#include "platform/PlatformConfig.h"
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
    , m_gearDropped(false)
    , m_aiMode(ChocoboAiMode::Wander)
    , m_inventory("Chocobo Saddlebags", InventoryChocoboSaddlebag::SADDLEBAG_SLOTS)
    , m_wingRotation(0.0f)
    , m_destPos(0.0f)
    , m_wingRotDelta(1.0f)
    , m_timeUntilNextFeather(600 + rand.nextInt(600))
    , m_flyParticleCooldown(0)
    , m_riderForward(0.0f)
    , m_riderStrafe(0.0f)
    , m_riderJump(false)
    , m_riderSneak(false)
    , m_sneakArmed(false)
    , m_jumpMoveFactor(0.02f)
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
    // Constants.CHOCOBO_<COLOUR>_CANJUMPHIGH: only White and Black.
    return m_color == ChocoboColor::White || m_color == ChocoboColor::Black;
}

bool EntityChocobo::canFly() const
{
    return m_color == ChocoboColor::Gold || m_color == ChocoboColor::Pink ||
           m_color == ChocoboColor::Red  || m_color == ChocoboColor::Purple;
}

bool EntityChocobo::canBreatheUnderwater()
{
    // Only EntityChocoboBlue overrides canBreatheUnderwater (func_70648_aU).
    return m_color == ChocoboColor::Blue;
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
    case ChocoboColor::Yellow: return spanish ? "Corredor clásico" : "Classic runner";
    case ChocoboColor::Green:  return spanish ? "Escala montañas" : "Climbs mountains";
    case ChocoboColor::Blue:   return spanish ? "Cruza agua + Resp. acuática" : "Crosses water + Water Breathing";
    case ChocoboColor::White:  return spanish ? "Escala, cruza agua, salta alto" : "Climbs, crosses water, high jump";
    case ChocoboColor::Black:  return spanish ? "Veloz, salta alto + Visión nocturna" : "Fast, high jump + Night Vision";
    case ChocoboColor::Gold:   return spanish ? "Vuela (A sube, L3 baja)" : "Flies (A up, L3 down)";
    case ChocoboColor::Pink:   return spanish ? "El más rápido, vuela (A/L3)" : "Fastest, flies (A/L3)";
    case ChocoboColor::Red:    return spanish ? "El más rápido, vuela (A/L3)" : "Fastest, flies (A/L3)";
    case ChocoboColor::Purple: return spanish ? "Vuela + Inmune al fuego" : "Flies + Fire immune";
    default:                   return "";
    }
}

std::string EntityChocobo::getRideHelp(bool spanish, bool longForm) const
{
#if PLATFORM_XBOX
    const std::string jumpKey = "A";
    const std::string sneakKey = "L3";
#else
    const std::string jumpKey = spanish ? "Saltar" : "Jump";
    const std::string sneakKey = spanish ? "Agacharse" : "Sneak";
#endif
    std::string s = spanish ? "Montado: " : "Riding: ";
    if (canFly())
    {
        if (spanish)
            s += longForm ? "mantén " + jumpKey + " para volar y subir, suelta para planear, " + sneakKey +
                                " para bajar; en el suelo " + sneakKey + " desmonta"
                          : jumpKey + " volar/subir, " + sneakKey + " bajar/desmontar";
        else
            s += longForm ? "hold " + jumpKey + " to fly up, release to glide, " + sneakKey +
                                " to go down; on the ground " + sneakKey + " dismounts"
                          : jumpKey + " fly/up, " + sneakKey + " down/dismount";
    }
    else
    {
        const std::string jump = canJumpHigh() ? (spanish ? " salto alto, " : " high jump, ")
                                               : (spanish ? " saltar, " : " jump, ");
        s += jumpKey + jump + sneakKey + (spanish ? " desmontar" : " dismount");
        if (longForm)
            s += spanish ? " (al caer, planea)" : " (glides when falling)";
    }
    return s;
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

// ChocoCraft 4.1.5 Constants.CHOCOBO_<COLOUR>_{LAND,WATER,AIR}SPEEDFACT / 100
float EntityChocobo::getLandSpeedFactor() const
{
    switch (m_color)
    {
    case ChocoboColor::Green:
    case ChocoboColor::Blue:   return 0.27f;
    case ChocoboColor::White:  return 0.35f;
    case ChocoboColor::Black:
    case ChocoboColor::Purple: return 0.40f;
    case ChocoboColor::Gold:   return 0.50f;
    case ChocoboColor::Pink:
    case ChocoboColor::Red:    return 0.55f;
    default:                   return 0.20f;
    }
}

float EntityChocobo::getWaterSpeedFactor() const
{
    switch (m_color)
    {
    case ChocoboColor::Blue:   return 0.55f;
    case ChocoboColor::White:  return 0.45f;
    case ChocoboColor::Black:
    case ChocoboColor::Gold:   return 0.20f;
    case ChocoboColor::Pink:
    case ChocoboColor::Red:    return 0.25f;
    default:                   return 0.10f;
    }
}

float EntityChocobo::getAirSpeedFactor() const
{
    switch (m_color)
    {
    case ChocoboColor::Gold:
    case ChocoboColor::Purple: return 0.55f;
    case ChocoboColor::Pink:
    case ChocoboColor::Red:    return 0.60f;
    default:                   return 0.0f;
    }
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
    m_riderJump = false;
    m_riderSneak = false;
    // The sneak that opened the chocobo menu (held, or still on with
    // "Sneak: Toggle") must not count as the dismount press.
    m_sneakArmed = false;
    if (EntityPlayerSP *sp = dynamic_cast<EntityPlayerSP *>(player))
    {
        if (MovementInputFromOptions *in = dynamic_cast<MovementInputFromOptions *>(sp->movementInput))
            in->clearSneakToggle();
    }
    applyMountedStepHeight(true);
    if (player->ridingEntity != this)
    {
        player->mountEntity(this);
        Minecraft *mc = Minecraft::getMinecraft();
        if (mc != nullptr && mc->ingameGUI != nullptr && dynamic_cast<EntityPlayerSP *>(player) != nullptr)
        {
            StringTranslate *tr = StringTranslate::getInstance();
            const bool es = tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
            mc->ingameGUI->addChatMessage(getRideHelp(es, true));
        }
    }
}

void EntityChocobo::dismountChocobo(EntityPlayer *player)
{
    isJumping = false;
    m_flying = false;
    m_isHighJumping = false;
    m_riderJump = false;
    m_riderSneak = false;
    fallDistance = 0.0f;
    if (EntityPlayerSP *sp = dynamic_cast<EntityPlayerSP *>(player))
    {
        if (MovementInputFromOptions *in = dynamic_cast<MovementInputFromOptions *>(sp->movementInput))
            in->clearSneakToggle();
    }
    applyMountedStepHeight(false);
    if (player != nullptr)
    {
        player->fallDistance = 0.0f;
        player->motionY = 0.0;
        player->mountEntity(nullptr);
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
    const float rad = (rotationYaw - 90.0f) * 3.14159265f / 180.0f;
    const double deltaX = static_cast<double>(MathHelper::cos(rad) * 0.4f);
    const double deltaZ = static_cast<double>(MathHelper::sin(rad) * 0.4f);
    riddenByEntity->setPosition(posX + deltaX, posY + getMountedYOffset() + riddenByEntity->getYOffset(), posZ + deltaZ);
    riddenByEntity->fallDistance = 0.0f;
}

void EntityChocobo::fall(float dist)
{
    if (riddenByEntity != nullptr)
        riddenByEntity->fallDistance = 0.0f;

    // Gold, Pink, Red and Purple override fall() with an empty body. A ridden
    // chocobo is also spared here (console rider safety; the original hurts it).
    if (canFly() || riddenByEntity != nullptr)
    {
        fallDistance = 0.0f;
        return;
    }

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
    if (riddenByEntity != nullptr)
    {
        // ChocoCraft EntityChocobo.updateEntityActionState (func_70626_be), ridden:
        // the chocobo faces where the rider looks, jumps with the rider's jump key
        // and its horizontal speed is capped at 0.375 blocks/tick.
        setPathToEntity(nullptr);
        playerToAttack = nullptr;
        randomYawVelocity = 0.0f;
        if (dynamic_cast<EntityPlayer *>(riddenByEntity) != nullptr)
        {
            prevRotationYaw = rotationYaw = riddenByEntity->rotationYaw;
            prevRotationPitch = rotationPitch = 0.0f;
            isJumping = m_riderJump;
            moveForward = m_riderForward;
            moveStrafing = m_riderStrafe * 0.5f;
            const double d = std::sqrt(motionX * motionX + motionZ * motionZ);
            if (d > 0.375)
            {
                const double scale = 0.375 / d;
                motionX *= scale;
                motionZ *= scale;
            }
        }
        else
        {
            moveForward = 0.0f;
            moveStrafing = 0.0f;
            isJumping = false;
        }
        return;
    }

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
            moveForward = 0.0f;
            moveStrafing = 0.0f;
            isJumping = isInWater();
            return;
        }
    }

    EntityAnimal::updatePlayerActionState();
}

void EntityChocobo::onLivingUpdate()
{
    if (!ModManager::getInstance().isModEnabled("chococraft"))
        return;
    const bool childNow = isChild();
    if (childNow != m_wasChild)
    {
        m_wasChild = childNow;
        if (childNow)
            setSize(0.5f, 0.5f);
        else
            growUpToAdult();
    }

    if (m_flyParticleCooldown > 0)
        --m_flyParticleCooldown;

    if (m_color == ChocoboColor::Purple)
    {
        extinguish();
        immuneToFire = true;
    }

    m_destPos += static_cast<float>(onGround ? -1 : 4) * 0.3f;
    m_destPos = std::clamp(m_destPos, 0.0f, 1.0f);
    if (!onGround && m_wingRotDelta < 1.0f)
        m_wingRotDelta = 1.0f;
    m_wingRotDelta *= 0.9f;
    m_wingRotation += m_wingRotDelta * 2.0f;


    if (!childNow && worldObj != nullptr && !worldObj->multiplayerWorld && --m_timeUntilNextFeather <= 0)
    {
        if (rand.nextInt(100) < 15 && ChocoCraftItems::chocoboFeather != nullptr)
            dropItem(ChocoCraftItems::chocoboFeather->shiftedIndex, 1);
        m_timeUntilNextFeather = 600 + rand.nextInt(600);
    }

    doStrawHealing();

    EntityPlayer *rider = dynamic_cast<EntityPlayer *>(riddenByEntity);
    if (rider != nullptr)
    {
        readRiderInput(rider);
        fallDistance = 0.0f;
        rider->fallDistance = 0.0f;
        applyRiderBuffs(rider);

        // Sneak (L3) dismounts, as sneaking does in 1.7.10 (EntityPlayer.updateRidden),
        // but only on a fresh press. A flyer in the air and a Blue chocobo under water
        // use it to go down instead (moveEntity), so it never drops the rider mid-air.
        const bool sneakMovesDown = (canFly() && !onGround && !isInWater()) ||
                                    (isInWater() && canBreatheUnderwater());
        if (!m_riderSneak)
            m_sneakArmed = true;
        else if (sneakMovesDown)
            m_sneakArmed = false;
        else if (m_sneakArmed)
        {
            dismountChocobo(rider);
            EntityAnimal::onLivingUpdate();
            return;
        }

        EntityAnimal::onLivingUpdate();

        fallDistance = 0.0f;
        if (riddenByEntity != nullptr)
            riddenByEntity->fallDistance = 0.0f;
        return;
    }

    EntityAnimal::onLivingUpdate();
}

void EntityChocobo::readRiderInput(EntityPlayer *rider)
{
    m_riderForward = 0.0f;
    m_riderStrafe = 0.0f;
    m_riderJump = false;
    m_riderSneak = false;
    if (rider == nullptr)
        return;
    if (EntityPlayerSP *sp = dynamic_cast<EntityPlayerSP *>(rider))
    {
        if (sp->movementInput != nullptr)
        {
            // Idempotent within a tick (the toggle-sneak edge is kept by the input).
            sp->movementInput->updatePlayerMoveState(sp);
            m_riderForward = sp->movementInput->moveForward;
            m_riderStrafe = sp->movementInput->moveStrafe;
            m_riderJump = sp->movementInput->jump;
            m_riderSneak = sp->movementInput->sneak;
        }
        return;
    }
    m_riderForward = rider->moveForward;
    m_riderStrafe = rider->moveStrafing;
    m_riderJump = rider->isJumping;
    m_riderSneak = rider->isSneaking();
}

void EntityChocobo::jump()
{
    EntityAnimal::jump();
    // 1.7.10 jump() sets isAirBorne and ChocoCraft then takes the colour's
    // airborne speed factor as jumpMovementFactor (0 for the non-flyers).
    if (riddenByEntity != nullptr)
        m_jumpMoveFactor = getAirSpeedFactor();
}

void EntityChocobo::moveEntityWithHeading(float strafe, float forward)
{
    if (dynamic_cast<EntityPlayer *>(riddenByEntity) != nullptr)
    {
        // ChocoCraft EntityChocobo.moveEntityWithHeading (func_70612_e) and
        // getAIMoveSpeed (func_70689_ay): the rider steers, strafing at half
        // speed, with the colour's land speed factor on the ground.
        strafe = m_riderStrafe * 0.5f;
        forward = m_riderForward;
        if (isInWater())
            m_jumpMoveFactor = getWaterSpeedFactor();
        if (canFly() && m_flying)
            m_jumpMoveFactor = getAirSpeedFactor();
        landMovementFactor = getLandSpeedFactor();
        jumpMovementFactor = m_jumpMoveFactor;
    }
    else
    {
        jumpMovementFactor = 0.02f;
    }
    EntityAnimal::moveEntityWithHeading(strafe, forward);
}

void EntityChocobo::moveEntity(double x, double y, double z)
{
    // ChocoCraft EntityChocoboRideable.moveEntity (func_70091_d). As in the
    // original, the new motionY applies from the next move on.
    if (dynamic_cast<EntityPlayer *>(riddenByEntity) != nullptr)
    {
        if (m_riderJump)
        {
            if (canFly())
            {
                motionY = 0.5;
                m_flying = true;
            }
            else if (canJumpHigh() && !m_isHighJumping)
            {
                motionY += 0.4;
                m_isHighJumping = true;
            }
        }
        else
        {
            motionY = -0.1;
        }

        if (m_riderSneak)
        {
            if (canFly())
                motionY = -0.5;
            if (inWater && canBreatheUnderwater())
                motionY -= 0.15;
        }
    }
    EntityAnimal::moveEntity(x, y, z);
}

void EntityChocobo::onUpdate()
{
    EntityAnimal::onUpdate();
    if (isDead || !ModManager::getInstance().isModEnabled("chococraft"))
        return;
    // ChocoCraft EntityChocobo.onUpdate (func_70071_h_)
    if (riddenByEntity == nullptr && m_flying && !onGround)
        motionY -= 0.25;
    if (m_flying && onGround)
        m_flying = false;
    if (m_isHighJumping && onGround)
        m_isHighJumping = false;
    if (isInWater() && canCrossWater())
    {
        motionY += 0.04;
        inWater = false;
    }
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

ChocoboColor EntityChocobo::breedColor(ChocoboColor a, ChocoboColor b, bool bothFedGold, Random &rng)
{
    const int roll = rng.nextInt(100);

    if (a == ChocoboColor::Purple || b == ChocoboColor::Purple)
    {
        ChocoboColor other = (a == ChocoboColor::Purple) ? b : a;
        if (other == ChocoboColor::Yellow || other == ChocoboColor::Green ||
            other == ChocoboColor::Blue   || other == ChocoboColor::White ||
            other == ChocoboColor::Black)
            return other;
        return ChocoboColor::Purple;
    }

    if (static_cast<int>(a) > static_cast<int>(b))
        std::swap(a, b);

    if ((a == ChocoboColor::Gold || a == ChocoboColor::Pink || a == ChocoboColor::Red) &&
        (b == ChocoboColor::Gold || b == ChocoboColor::Pink || b == ChocoboColor::Red))
    {
        return (bothFedGold && roll <= 20) ? ChocoboColor::Gold : ChocoboColor::Yellow;
    }

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

    if (b == ChocoboColor::Gold || b == ChocoboColor::Pink || b == ChocoboColor::Red)
    {
        if (bothFedGold && a == ChocoboColor::White && roll > 70) return ChocoboColor::Gold;
        if (bothFedGold && a == ChocoboColor::Black && roll > 90) return ChocoboColor::Gold;
        return a;
    }

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
    if (player == nullptr || worldObj == nullptr || !ModManager::getInstance().isModEnabled("chococraft"))
        return false;

    ItemStack *held = player->inventory ? player->inventory->getCurrentItem() : nullptr;

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
        m_fedGold = false;
        return EntityAnimal::interact(player);
    }

    const bool isLoverly = held != nullptr && ChocoCraftItems::gysahlLoverly != nullptr &&
                           held->itemID == ChocoCraftItems::gysahlLoverly->shiftedIndex;
    const bool isGolden = held != nullptr && ChocoCraftItems::gysahlGolden != nullptr &&
                          held->itemID == ChocoCraftItems::gysahlGolden->shiftedIndex;
    if ((isLoverly || isGolden) && m_tamed && !isChild())
    {
        m_fedGold = isGolden;
        return EntityAnimal::interact(player);
    }

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

    // String (silk): ChocoCraft onSilkUse -> toggleFollowStay
    if (held != nullptr && Item::silk != nullptr && held->itemID == Item::silk->shiftedIndex)
    {
        if (m_tamed)
        {
            m_aiMode = (m_aiMode == ChocoboAiMode::Stay) ? ChocoboAiMode::Follow : ChocoboAiMode::Stay;
            spawnParticles(m_aiMode == ChocoboAiMode::Follow ? "heart" : "smoke", 7);
            return true;
        }
        spawnParticles("smoke", 7);
        return true;
    }

    const bool isSaddleItem = held != nullptr && (
        (ChocoCraftItems::chocoboSaddle != nullptr && held->itemID == ChocoCraftItems::chocoboSaddle->shiftedIndex) ||
        (Item::saddle != nullptr && held->itemID == Item::saddle->shiftedIndex));
    if (isSaddleItem)
    {
        if (m_tamed && !isChild() && !m_saddled && !m_packbags)
        {
            setSaddled(true);
            m_aiMode = ChocoboAiMode::Stay;
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

    // The rider cannot aim at its own chocobo (canRiderInteract); never let a
    // stray click on it toggle the mount off.
    if (riddenByEntity == player)
        return true;

    if (m_tamed && m_saddled && !m_packbags && !isChild() && riddenByEntity == nullptr)
    {
        mountChocobo(player);
        return true;
    }

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
