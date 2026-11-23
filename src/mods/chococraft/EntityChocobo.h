#pragma once

#include "net/minecraft/src/EntityAnimal.h"
#include "net/minecraft/src/IInventory.h"
#include <string>

class World;
class EntityPlayer;
class ItemStack;
class NBTTagCompound;

// ChocoCraft 4.1.5 (1.7.10) 9 chocobo colours
enum class ChocoboColor : int
{
    Yellow = 0, // Classic runner
    Green  = 1, // Climbs mountains (stepHeight 2.0) & high jump
    Blue   = 2, // Fast swimmer, floats on water, Water Breathing buff
    White  = 3, // Climbs, swims fast, high jump
    Black  = 4, // Very fast, climbs, swims, high jump, Night Vision buff, no fall dmg
    Gold   = 5, // Flies (A = up, L3 = down in air), climbs, swims, no fall dmg
    Pink   = 6, // Painted Gold: fastest flyer, climbs, swims, no fall dmg
    Red    = 7, // Painted Gold: fastest flyer, climbs, swims, no fall dmg
    Purple = 8  // Nether Chocobo: flies, fire & lava immune, Fire Resistance buff
};

enum class ChocoboAiMode : int
{
    Wander = 0,
    Follow = 1,
    Stay   = 2
};

class InventoryChocoboSaddlebag : public IInventory
{
public:
    static constexpr int_t SADDLEBAG_SLOTS = 27;
    static constexpr int_t PACKBAG_SLOTS   = 54;
    static constexpr int_t MAX_SLOTS       = 54;

    explicit InventoryChocoboSaddlebag(const std::string &title = "Chocobo Saddlebags", int_t slotCount = SADDLEBAG_SLOTS);
    ~InventoryChocoboSaddlebag() override;

    void setTitle(const std::string &title) { m_title = title; }
    void setSlotCount(int_t count);
    ItemStack *getStackInSlot(int_t slot) override;
    ItemStack *decrStackSize(int_t slot, int_t amount) override;
    ItemStack *getStackInSlotOnClosing(int_t slot) override;
    void setInventorySlotContents(int_t slot, ItemStack *stack) override;
    int_t getSizeInventory() override { return m_slotCount; }
    std::string getInvName() override { return m_title; }
    int_t getInventoryStackLimit() override { return 64; }
    void onInventoryChanged() override {}
    bool canInteractWith(EntityPlayer *player) override;

    void writeToNBT(NBTTagCompound *nbt) const;
    void readFromNBT(NBTTagCompound *nbt);
    void dropAllContents(World *world, double x, double y, double z);

private:
    std::string m_title;
    int_t m_slotCount;
    ItemStack *m_slots[MAX_SLOTS];
};

class EntityChocobo : public EntityAnimal
{
public:
    explicit EntityChocobo(World *world);
    ~EntityChocobo() override = default;

    // Colour & abilities (ChocoCraft 4.1.5)
    ChocoboColor getColor() const { return m_color; }
    void setColor(ChocoboColor color);
    static const char *getColorName(ChocoboColor color, bool spanish);
    static const char *getColorAbility(ChocoboColor color, bool spanish);
    static const char *getColorTextureFile(ChocoboColor color, bool isChicobo);

    bool canClimb() const;
    bool canCrossWater() const;
    bool canJumpHigh() const;
    bool canFly() const;

    bool isMale() const { return m_isMale; }
    void setMale(bool male) { m_isMale = male; }

    bool isTamed() const { return m_tamed; }
    void setTamed(bool tamed) { m_tamed = tamed; }
    const std::string &getOwnerName() const { return m_ownerName; }
    void setOwnerName(const std::string &name) { m_ownerName = name; }

    bool isSaddled() const { return m_saddled; }
    void setSaddled(bool saddled);

    bool hasSaddlebags() const { return m_saddlebags; }
    void setSaddlebags(bool bags);

    bool hasPackbags() const { return m_packbags; }
    void setPackbags(bool pack);

    bool isHidename() const { return m_hidename; }
    void setHidename(bool hide) { m_hidename = hide; }

    const std::string &getCustomName() const { return m_customName; }
    void setCustomName(const std::string &name);

    ChocoboAiMode getAiMode() const { return m_aiMode; }
    void setAiMode(ChocoboAiMode mode) { m_aiMode = mode; }

    bool isFlying() const { return m_flying; }
    void setFlying(bool flying) { m_flying = flying; }

    float getWingRotation() const { return m_wingRotation; }
    float getDestPos() const { return m_destPos; }

    std::string getChocoboTexturePath();
    InventoryChocoboSaddlebag *getSaddlebagInventory() { return &m_inventory; }

    void mountChocobo(EntityPlayer *player);
    void dismountChocobo(EntityPlayer *player);
    bool teleportToOwner(EntityPlayer *owner);

    // EntityLiving / EntityAnimal overrides
    int_t getMaxHealth() const override;
    bool canDespawn() override { return !m_tamed && !m_saddled && !m_packbags && !isChild(); }
    bool isAIEnabled() override { return false; }
    bool canBreatheUnderwater() override;
    void updatePlayerActionState() override;
    void onLivingUpdate() override;
    void updateRiderPosition() override;
    double getMountedYOffset() override { return isChild() ? 0.65 : 1.65; }
    void fall(float dist) override;
    bool interact(EntityPlayer *player) override;
    bool isWheat(ItemStack *itemstack) const override;
    EntityAnimal *spawnBabyAnimal(EntityAnimal *mate) override;
    int_t getDropItemId() override;
    void onDeath(Entity *entity) override;
    void onDeath(const DamageSource &source) override;
    void writeEntityToNBT(NBTTagCompound *nbt) override;
    void readEntityFromNBT(NBTTagCompound *nbt) override;

protected:
    jstring getLivingSound() override;
    jstring getHurtSound() override;
    jstring getDeathSound() override;
    float getSoundVolume() override { return 0.65f; }
    float getSoundPitch() override;

private:
    static ChocoboColor breedColor(ChocoboColor a, ChocoboColor b, bool bothFedGold, Random &rng);
    float getRideSpeed() const;
    float getWaterRideSpeed() const;
    float getAirRideSpeed() const;
    float getJumpVelocity() const;
    void applyMountedStepHeight(bool mounted);
    void applyRiderBuffs(EntityPlayer *rider);
    void doStrawHealing();
    void dropAllGearAndBags();
    void spawnParticles(const char *name, int count);
    void growUpToAdult();

    ChocoboColor m_color;
    bool m_isMale;
    bool m_tamed;
    bool m_saddled;
    bool m_saddlebags;
    bool m_packbags;
    bool m_hidename;
    bool m_fedGold;
    bool m_wasChild;
    bool m_flying;
    bool m_isHighJumping;
    bool m_gliding;
    bool m_gearDropped;
    ChocoboAiMode m_aiMode;
    std::string m_ownerName;
    std::string m_customName;
    InventoryChocoboSaddlebag m_inventory;
    float m_wingRotation;
    float m_destPos;
    float m_wingRotDelta;
    int_t m_timeUntilNextFeather;
    int_t m_mountGraceTicks;
    int_t m_flyParticleCooldown;
};
