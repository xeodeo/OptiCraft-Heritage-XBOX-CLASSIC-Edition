#pragma once

#include "net/minecraft/src/RenderLiving.h"
#include "EntityChocobo.h"

class ModelChocobo;
class ModelChicobo;

class RenderChocobo : public RenderLiving
{
public:
    RenderChocobo();
    ~RenderChocobo() override;

protected:
    float getTicksExisted(EntityLiving *entityliving, float partialTick) override;
    void preRenderCallback(EntityLiving *entityliving, float partialTick) override;
    void renderModel(EntityLiving *entityliving, float f, float f1, float f2, float f3, float f4, float f5) override;
    void passSpecialRender(EntityLiving *entityliving, double d, double d1, double d2) override;

private:
    ModelChocobo *m_adultModel;
    ModelChicobo *m_chicoboModel;
};
