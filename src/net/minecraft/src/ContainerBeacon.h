#pragma once
#include "Container.h"
class InventoryPlayer;
class TileEntityBeacon;
class ContainerBeacon : public Container {
public:
    ContainerBeacon(InventoryPlayer *p, TileEntityBeacon *b);
    bool isUsableByPlayer(EntityPlayer *p) override;
    bool enchantItem(EntityPlayer *p, int_t id) override;
    TileEntityBeacon *getBeacon() const;
    int getBeaconLevel() const;
    int getPrimaryEffect() const;
    int getSecondaryEffect() const;
    bool isPayment(ItemStack *stack) const;
    bool canSelectEffect(int id) const;
    bool selectPrimaryEffect(int id);
    bool selectSecondaryEffect(int id);
    ItemStack *getPayment() const;
    bool hasPayment() const;
    void clearPayment();
    bool hasBeacon() const;
    bool paymentMatches(ItemStack *stack) const;
    bool validPrimary(int id) const;
    bool validSecondary(int id) const;
    int effectTier(int id) const;
    int effectId(int id) const;
    void resetEffects();
    bool isInRange(EntityPlayer *p) const;
    bool isOpen() const;
    bool hasPrimaryEffect() const;
    bool hasSecondaryEffect() const;
    bool canPay() const;
    int paymentItemId() const;
private:
    TileEntityBeacon *beacon;
};
