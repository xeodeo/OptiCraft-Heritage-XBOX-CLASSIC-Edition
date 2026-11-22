#include "RenderChocobo.h"
#include "ModelChocobo.h"
#include "EntityChocobo.h"

#include "net/minecraft/src/RenderManager.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/MathHelper.h"
#include "java/BufferedImage.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
struct ChocoboPalette
{
    int_t featherMain;
    int_t featherLight;
    int_t featherDark;
    int_t crestTip;
    int_t beakMain;
    int_t beakDark;
    int_t legColor;
    int_t eyeColor;
};

static const ChocoboPalette PALETTES[6] = {
    // 0: Yellow (Classic)
    { static_cast<int_t>(0xFFF2CC30u), static_cast<int_t>(0xFFFFE566u), static_cast<int_t>(0xFFC99E18u),
      static_cast<int_t>(0xFFFFEE88u), static_cast<int_t>(0xFFE88A24u), static_cast<int_t>(0xFFC06818u),
      static_cast<int_t>(0xFFD98A32u), static_cast<int_t>(0xFF183868u) },
    // 1: Green (Mountain)
    { static_cast<int_t>(0xFF3E9E44u), static_cast<int_t>(0xFF62C864u), static_cast<int_t>(0xFF28722Eu),
      static_cast<int_t>(0xFF90E878u), static_cast<int_t>(0xFFE09228u), static_cast<int_t>(0xFFB86C18u),
      static_cast<int_t>(0xFF8C7A42u), static_cast<int_t>(0xFF182818u) },
    // 2: Blue (River/Ocean)
    { static_cast<int_t>(0xFF3478D8u), static_cast<int_t>(0xFF60A8FFu), static_cast<int_t>(0xFF20509Eu),
      static_cast<int_t>(0xFF98D4FFu), static_cast<int_t>(0xFFE8982Eu), static_cast<int_t>(0xFFBC741Cu),
      static_cast<int_t>(0xFF6C8AB0u), static_cast<int_t>(0xFF102040u) },
    // 3: White (Agile)
    { static_cast<int_t>(0xFFEAECEFu), static_cast<int_t>(0xFFFFFFFFu), static_cast<int_t>(0xFFC0C6CEu),
      static_cast<int_t>(0xFFFFFFFFu), static_cast<int_t>(0xFFE89B38u), static_cast<int_t>(0xFFC27A20u),
      static_cast<int_t>(0xFFD49A52u), static_cast<int_t>(0xFF2266CCu) },
    // 4: Black (Swift)
    { static_cast<int_t>(0xFF2A2B32u), static_cast<int_t>(0xFF424552u), static_cast<int_t>(0xFF181920u),
      static_cast<int_t>(0xFF565A6Cu), static_cast<int_t>(0xFFD88424u), static_cast<int_t>(0xFFA86014u),
      static_cast<int_t>(0xFF4A4440u), static_cast<int_t>(0xFFFFCC22u) },
    // 5: Gold (Legendary Flyer)
    { static_cast<int_t>(0xFFFFC818u), static_cast<int_t>(0xFFFFF280u), static_cast<int_t>(0xFFD49408u),
      static_cast<int_t>(0xFFFFFFAAu), static_cast<int_t>(0xFFFF7A18u), static_cast<int_t>(0xFFCC5808u),
      static_cast<int_t>(0xFFE8A020u), static_cast<int_t>(0xFF1888FFu) }
};

void setPx(std::vector<int_t> &px, int_t x, int_t y, int_t col)
{
    if (x >= 0 && x < 64 && y >= 0 && y < 32)
        px[static_cast<size_t>(y) * 64 + static_cast<size_t>(x)] = col;
}

void fillBox(std::vector<int_t> &px, int_t x0, int_t y0, int_t w, int_t h, int_t col, int_t speckle)
{
    for (int_t y = y0; y < y0 + h; ++y)
    {
        for (int_t x = x0; x < x0 + w; ++x)
        {
            const bool alt = (((x * 13 + y * 7) & 7) == 0);
            setPx(px, x, y, alt ? speckle : col);
        }
    }
}

void generateChocoboTexture(ChocoboColor color, std::vector<int_t> &px)
{
    const int idx = std::clamp<int>(static_cast<int>(color), 0, 5);
    const ChocoboPalette &p = PALETTES[idx];
    px.assign(64 * 32, p.featherMain);

    // Subtle feather grain across the whole base sheet
    fillBox(px, 0, 0, 64, 32, p.featherMain, p.featherLight);

    // Head region (0..24, 0..12)
    fillBox(px, 0, 0, 24, 12, p.featherLight, p.featherMain);
    // Front face of head box (6x6x6 at uv 0,0 -> front face is u=[6..12), v=[6..12))
    // Side faces: right u=[0..6), left u=[12..18)
    // Bright eyes with white glint on side-front corners
    setPx(px, 6, 8, static_cast<int_t>(0xFFFFFFFFu));
    setPx(px, 7, 8, p.eyeColor);
    setPx(px, 10, 8, p.eyeColor);
    setPx(px, 11, 8, static_cast<int_t>(0xFFFFFFFFu));
    setPx(px, 4, 8, p.eyeColor);
    setPx(px, 13, 8, p.eyeColor);

    // Neck (24..40, 0..13)
    fillBox(px, 24, 0, 16, 13, p.featherMain, p.featherLight);

    // Beak (42..54, 0..6)
    fillBox(px, 42, 0, 12, 6, p.beakMain, p.beakDark);

    // Crest feathers (52..64, 0..8)
    fillBox(px, 52, 0, 12, 8, p.crestTip, p.featherLight);

    // Wings (36..56, 6..21) - darker feather tips
    fillBox(px, 36, 6, 18, 14, p.featherDark, p.featherMain);

    // Body (0..44, 12..32)
    fillBox(px, 0, 12, 44, 12, p.featherMain, p.featherDark);

    // Tail feathers (36..64, 18..28)
    fillBox(px, 36, 18, 28, 10, p.featherDark, p.crestTip);

    // Legs & Feet (52..64, 10..24) & (42..60, 24..32)
    fillBox(px, 52, 10, 12, 14, p.legColor, p.beakDark);
    fillBox(px, 42, 24, 18, 8, p.beakDark, p.legColor);

    // Saddle (0..28, 24..32) - classic warm leather + gold buckle
    const int_t leather = static_cast<int_t>(0xFF7A4420u);
    const int_t leatherDark = static_cast<int_t>(0xFF562E14u);
    const int_t buckle = static_cast<int_t>(0xFFE8C030u);
    fillBox(px, 0, 24, 28, 8, leather, leatherDark);
    fillBox(px, 11, 26, 4, 3, buckle, buckle);

    // Saddlebags (28..42, 22..32) - stitched travel pouches
    const int_t bagMain = static_cast<int_t>(0xFF8B5628u);
    const int_t bagTrim = static_cast<int_t>(0xFF4B2B12u);
    fillBox(px, 28, 22, 14, 10, bagMain, bagTrim);
    fillBox(px, 32, 24, 3, 2, buckle, buckle);
}
} // namespace

RenderChocobo::RenderChocobo()
    : RenderLiving(new ModelChocobo(), 0.65f)
{
    m_chocoModel = static_cast<ModelChocobo *>(mainModel);
    for (int i = 0; i < 6; ++i)
        m_colorTexIds[i] = -1;
}

RenderChocobo::~RenderChocobo()
{
    if (renderManager != nullptr && renderManager->renderEngine != nullptr)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (m_colorTexIds[i] >= 0)
            {
                renderManager->renderEngine->deleteTexture(m_colorTexIds[i]);
                m_colorTexIds[i] = -1;
            }
        }
    }
}

int_t RenderChocobo::getTextureForColor(RenderEngine *engine, ChocoboColor color)
{
    const int idx = std::clamp<int>(static_cast<int>(color), 0, 5);
    if (m_colorTexIds[idx] < 0 && engine != nullptr)
    {
        std::vector<int_t> pixels;
        generateChocoboTexture(color, pixels);
        std::vector<unsigned char> rgba(64 * 32 * 4, 255);
        for (size_t i = 0; i < 64 * 32; ++i)
        {
            const uint32_t argb = static_cast<uint32_t>(pixels[i]);
            rgba[i * 4 + 0] = static_cast<unsigned char>((argb >> 16) & 0xFF);
            rgba[i * 4 + 1] = static_cast<unsigned char>((argb >> 8) & 0xFF);
            rgba[i * 4 + 2] = static_cast<unsigned char>(argb & 0xFF);
            rgba[i * 4 + 3] = static_cast<unsigned char>((argb >> 24) & 0xFF);
        }
        BufferedImage img(64, 32);
        img.setRGB(0, 0, 64, 32, rgba.data());
        m_colorTexIds[idx] = engine->allocateAndSetupTexture(&img);
    }
    return m_colorTexIds[idx];
}

float RenderChocobo::getTicksExisted(EntityLiving *entityliving, float partialTick)
{
    EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entityliving);
    if (choco == nullptr)
        return 0.0f;
    const float flap = choco->getPrevWingFlap() + (choco->getWingFlap() - choco->getPrevWingFlap()) * partialTick;
    return (MathHelper::sin(flap) + 1.0f) * (choco->onGround ? 0.08f : 0.72f);
}

void RenderChocobo::renderModel(EntityLiving *entityliving, float f, float f1, float f2, float f3, float f4, float f5)
{
    if (renderManager == nullptr || renderManager->renderEngine == nullptr)
        return;

    EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entityliving);
    ChocoboColor color = choco ? choco->getColor() : ChocoboColor::Yellow;
    if (m_chocoModel != nullptr)
    {
        m_chocoModel->showSaddle = choco != nullptr && choco->isSaddled();
        m_chocoModel->showSaddlebags = choco != nullptr && choco->hasSaddlebags();
    }

    const int_t texId = getTextureForColor(renderManager->renderEngine, color);
    if (texId >= 0)
        renderManager->renderEngine->bindTexture(texId);

    mainModel->render(entityliving, f, f1, f2, f3, f4, f5);
}

void RenderChocobo::passSpecialRender(EntityLiving *entityliving, double d, double d1, double d2)
{
    RenderLiving::passSpecialRender(entityliving, d, d1, d2);
    EntityChocobo *choco = dynamic_cast<EntityChocobo *>(entityliving);
    if (choco != nullptr && !choco->getCustomName().empty() && choco->riddenByEntity != renderManager->livingPlayer)
    {
        renderLivingLabel(entityliving, choco->getCustomName(), d, d1 + 0.25, d2, 32);
    }
}
