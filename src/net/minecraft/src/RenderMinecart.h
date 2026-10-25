#ifndef RENDERMINE_H
#define RENDERMINE_H

#include "Render.h"

class Block;
class EntityMinecart;
class ModelBase;

class RenderMinecart : public Render {
public:
    RenderMinecart();
    ~RenderMinecart() override;

    void renderMinecart(EntityMinecart* entityMinecart, double d, double d1, double d2, float f, float f1);

    virtual void doRender(Entity* entity, double d, double d1, double d2, float f, float f1) override;

protected:
    virtual void renderBlockInMinecart(EntityMinecart* entityMinecart, float partialTicks, Block* block, int_t metadata);

    ModelBase* modelMinecart;
};

#endif
