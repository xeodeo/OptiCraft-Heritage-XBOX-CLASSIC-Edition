#pragma once

#include "net/minecraft/src/RenderLiving.h"
#include "EntityChocobo.h"

class ModelChocobo;
class RenderEngine;

class RenderChocobo : public RenderLiving
{
public:
    RenderChocobo();
    ~RenderChocobo() override;

protected:
    float getTicksExisted(EntityLiving *entityliving, float partialTick) override;
    void renderModel(EntityLiving *entityliving, float f, float f1, float f2, float f3, float f4, float f5) override;
    void passSpecialRender(EntityLiving *entityliving, double d, double d1, double d2) override;

private:
    int_t getTextureForColor(RenderEngine *engine, ChocoboColor color);

    ModelChocobo *m_chocoModel;
    int_t m_colorTexIds[6];
};
