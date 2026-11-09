#ifndef RENDERWOLF_H
#define RENDERWOLF_H

#include "RenderLiving.h"

class EntityWolf;
class ModelBase;

class RenderWolf : public RenderLiving {
public:
    RenderWolf(ModelBase* modelBase, ModelBase* renderPassModel, float shadowSize);
    ~RenderWolf() override;

    void renderWolf(EntityWolf* entityWolf, double d, double d1, double d2, float f, float f1);

    virtual void doRenderLiving(EntityLiving* entityLiving, double d, double d1, double d2, float f, float f1) override;

    virtual void doRender(Entity* entity, double d, double d1, double d2, float f, float f1) override;

protected:
    float getTicksExisted(EntityLiving* entityLiving, float f) override;
    int shouldRenderPass(EntityLiving* entityLiving, int pass, float partialTicks) override;
    int func_82447_a(EntityWolf* wolf, int pass, float partialTicks);

    virtual void preRenderCallback(EntityLiving* entityLiving, float f) override;

};

#endif
