#pragma once

#include "net/minecraft/src/RenderBiped.h"

class ModelBiped;
class RenderEngine;

class RenderHerobrine : public RenderBiped
{
public:
    RenderHerobrine();
    ~RenderHerobrine() override;

protected:
    void renderModel(EntityLiving *entityliving, float f, float f1, float f2, float f3, float f4, float f5) override;

private:
    void ensureTextures(RenderEngine *engine);

    ModelBiped *m_eyeModel;
    int_t m_skinTexId;
    int_t m_eyesTexId;
};
