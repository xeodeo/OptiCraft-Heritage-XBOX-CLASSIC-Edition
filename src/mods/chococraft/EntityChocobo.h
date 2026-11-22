#pragma once

#include "net/minecraft/src/EntityAnimal.h"
#include "net/minecraft/src/IInventory.h"
#include <string>

class World;
class EntityPlayer;
class ItemStack;
class NBTTagCompound;

enum class ChocoboColor : int
{
    Yellow = 0, // Classic runner
    Green  = 1, // Climbs mountains (stepHeight 1.6)
    Blue   = 2, // Swims fast & walks across water
    White  = 3, // Agile high jump & gentle fall
    Black  = 4, // Fastest runner, no fall damage
    Gold   = 5  // Flies with A button, climbs, swims, no fall damage
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
    static constexpr int_t SLOT_COUNT = 27;

    explicit InventoryChocoboSaddlebag(const std::string &title = "Chocobo Saddlebags");
    ~InventoryChocoboSaddlebag() override;

    void setTitle(const std::string &title) { m_title = title; }
    ItemStack *getStackInSlot(int_t slot) override;
    ItemStack *decrStackSize(int_t slot, int_t amount) override;
    ItemStack *getStackInSlotOnClosing(int_t slot) override;
    void setInventorySlotContents(int_t slot, ItemStack *stack) override;
    int_t getSizeInventory() override { return SLOT_COUNT; }
    std::string getInvName() override { return m_title; }
    int_t getInventoryStackLimit() override { return 64; }
    void onInventoryChanged() override {}
    bool canInteractWith(EntityPlayer *player) override;

    void writeToNBT(NBTTagCompound *nbt) const;
    void readFromNBT(NBTTagCompound *nbt);
    void dropAllContents(World *world, double x, double y, double z);

private:
    std::string m_title;
    ItemStack *m_slots[SLOT_COUNT];
};

class EntityChocobo : public EntityAnimal
{
public:
    explicit EntityChocobo(World *world);
    ~EntityChocobo() override = default;

    // Color & attributes
    ChocoboColor getColor() const { return m_color; }
    void setColor(ChocoboColor color);
    static const char *getColorName(ChocoboColor color, bool spanish);
    static const char *getColorAbility(ChocoboColor color, bool spanish);

    bool isTamed() const { return m_tamed; }
    void setTamed(bool tamed) { m_tamed = tamed; }
    const std::string &getOwnerName() const { return m_ownerName; }
    void setOwnerName(const std::string &name) { m_ownerName = name; }

    bool isSaddled() const { return m_saddled; }
    void setSaddled(bool saddled) { m_saddled = saddled; }

    bool hasSaddlebags() const { return m_saddlebags; }
    void setSaddlebags(bool bags) { m_saddlebags = bags; }

    const std::string &getCustomName() const { return m_customName; }
    void setCustomName(const std::string &name);

    ChocoboAiMode getAiMode() const { return m_aiMode; }
    void setAiMode(ChocoboAiMode mode) { m_aiMode = mode; }

    InventoryChocoboSaddlebag *getSaddlebagInventory() { return &m_inventory; }
    float getWingFlap() const { return m_wingFlap; }
    float getPrevWingFlap() const { return m_prevWingFlap; }

    // EntityLiving / EntityAnimal overrides
    int_t getMaxHealth() const override;
    bool canDespawn() override { return !m_tamed; }
    void onLivingUpdate() override;
    void updateRiderPosition() override;
    double getMountedYOffset() override { return 1.12; }
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
    jstring getLivingSound() override { return "mob.chicken"; }
    jstring getHurtSound() override { return "mob.chickenhurt"; }
    jstring getDeathSound() override { return "mob.chickenhurt"; }
    float getSoundVolume() override { return 0.6f; }

private:
    static ChocoboColor breedColor(ChocoboColor a, ChocoboColor b, Random &rng);
    float getRideSpeed() const;
    float getJumpVelocity() const;
    void spawnParticles(const char *name, int count);

    ChocoboColor m_color;
    bool m_tamed;
    bool m_saddled;
    bool m_saddlebags;
    ChocoboAiMode m_aiMode;
    std::string m_ownerName;
    std::string m_customName;
    InventoryChocoboSaddlebag m_inventory;
    float m_wingFlap;
    float m_prevWingFlap;
    float m_wingFlapSpeed;
    int_t m_flyCooldown;
};
