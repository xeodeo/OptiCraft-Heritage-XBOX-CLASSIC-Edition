#include "RenderChocobo.h"
#include "ModelChocobo.h"
#include "EntityChocobo.h"

#include "net/minecraft/src/RenderManager.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/MathHelper.h"
#include "platform/RenderAPI.h"

RenderChocobo::RenderChocobo()
    : RenderLiving(new ModelChocobo(), 0.65f)
    , m_adultModel(nullptr)
    , m_chicoboModel(new ModelChicobo())
{
    m_adultModel = static_cast<ModelChocobo *>(mainModel);
}

RenderChocobo::~RenderChocobo()
{
    delete m_chicoboModel;
    m_chicoboModel = nullptr;
}

float RenderChocobo::getTicksExisted(EntityLiving *entityliving, float partialTick)
{
    (void)partialTick;
    EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entityliving);
    if (choco == nullptr)
        return 0.0f;
    return (MathHelper::sin(choco->getWingRotation()) + 1.0f) * choco->getDestPos();
}

void RenderChocobo::preRenderCallback(EntityLiving *entityliving, float partialTick)
{
    (void)partialTick;
    EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entityliving);
    if (choco != nullptr && choco->isChild())
    {
        // ChocoCraft 4.1.5 RenderChicobo.preRenderScale
        const float scale = 1.32f;
        renderScale(scale, scale, scale);
    }
    else
    {
        // ChocoCraft 4.1.5 RenderChocobo.preRenderScale
        const float scale = 0.8f;
        renderScale(scale, scale, scale);
    }
}

void RenderChocobo::renderModel(EntityLiving *entityliving, float f, float f1, float f2, float f3, float f4, float f5)
{
    if (renderManager == nullptr || renderManager->renderEngine == nullptr)
        return;

    EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entityliving);
    const std::string texPath = (choco != nullptr)
        ? choco->getChocoboTexturePath()
        : std::string("/chococraft/textures/entities/Chocobos/Untamed/Male/yellowchocobo.png");

    const int_t texId = renderManager->renderEngine->getTexture(texPath);
    if (texId >= 0)
        renderManager->renderEngine->bindTexture(texId);

    if (choco != nullptr && choco->isChild())
    {
        if (m_chicoboModel != nullptr)
            m_chicoboModel->render(entityliving, f, f1, f2, f3, f4, f5);
    }
    else
    {
        if (m_adultModel != nullptr)
        {
            m_adultModel->showSaddleBags = (choco != nullptr && choco->hasSaddlebags());
            m_adultModel->showPackBag = (choco != nullptr && choco->hasPackbags());
            m_adultModel->render(entityliving, f, f1, f2, f3, f4, f5);
        }
    }
}

void RenderChocobo::passSpecialRender(EntityLiving *entityliving, double d, double d1, double d2)
{
    RenderLiving::passSpecialRender(entityliving, d, d1, d2);
    EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entityliving);
    if (choco != nullptr && !choco->getCustomName().empty() && !choco->isHidename() &&
        choco->riddenByEntity != renderManager->livingPlayer)
    {
        renderLivingLabel(entityliving, choco->getCustomName(), d, d1 + (choco->isChild() ? 0.0 : 0.35), d2, 24);
    }
}
