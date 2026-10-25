#pragma once
#include "EntityMinecart.h"
class EntityMinecartTNT : public EntityMinecart {
public:
    EntityMinecartTNT(World *w); EntityMinecartTNT(World *w,double x,double y,double z);
    int_t getMinecartType() const override { return 3; }
    void onUpdate() override;
    void onActivatorRailPass(int_t x, int_t y, int_t z, bool powered) override;
private: int_t fuse = -1;
};
