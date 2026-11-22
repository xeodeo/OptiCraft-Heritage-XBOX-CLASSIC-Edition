#include "RenderHerobrine.h"
#include "EntityHerobrine.h"

#include "net/minecraft/src/ModelBiped.h"
#include "net/minecraft/src/RenderManager.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/OpenGlHelper.h"
#include "java/BufferedImage.h"
#include "platform/RenderAPI.h"

#include <vector>

namespace
{
void fillRect(std::vector<int_t> &px, int_t w, int_t h, int_t x0, int_t y0, int_t rw, int_t rh, int_t argb)
{
    for (int_t y = y0; y < y0 + rh; ++y)
    {
        if (y < 0 || y >= h) continue;
        for (int_t x = x0; x < x0 + rw; ++x)
        {
            if (x < 0 || x >= w) continue;
            px[static_cast<size_t>(y) * w + static_cast<size_t>(x)] = argb;
        }
    }
}

void buildFallbackSteveWithWhiteEyes(std::vector<int_t> &px, int_t w, int_t h)
{
    px.assign(static_cast<size_t>(w * h), 0);
    const int_t skin = static_cast<int_t>(0xFFC69674u);
    const int_t hair = static_cast<int_t>(0xFF3B2414u);
    const int_t beard = static_cast<int_t>(0xFF7B4E34u);
    const int_t shirt = static_cast<int_t>(0xFF00A8A8u);
    const int_t pants = static_cast<int_t>(0xFF333399u);
    const int_t shoes = static_cast<int_t>(0xFF4A4A4Au);

    // Head (8x8 faces in 0..32 x 0..16)
    fillRect(px, w, h, 0, 8, 32, 8, skin);
    fillRect(px, w, h, 8, 0, 8, 8, hair);
    fillRect(px, w, h, 16, 0, 8, 8, skin);
    fillRect(px, w, h, 0, 8, 32, 2, hair);
    fillRect(px, w, h, 24, 8, 8, 6, hair);
    // Nose & mouth/beard on face (8..16, 8..16)
    fillRect(px, w, h, 11, 13, 2, 1, static_cast<int_t>(0xFF9E6B4Eu));
    fillRect(px, w, h, 10, 14, 4, 2, beard);
    // Body (16..40, 16..32)
    fillRect(px, w, h, 16, 16, 24, 16, shirt);
    // Arms (40..56, 16..32)
    fillRect(px, w, h, 40, 16, 16, 8, shirt);
    fillRect(px, w, h, 40, 24, 16, 8, skin);
    // Legs (0..16, 16..32)
    fillRect(px, w, h, 0, 16, 16, 14, pants);
    fillRect(px, w, h, 0, 30, 16, 2, shoes);
}
} // namespace

RenderHerobrine::RenderHerobrine()
    : RenderBiped(new ModelBiped(0.0f), 0.5f)
    , m_eyeModel(new ModelBiped(0.02f))
    , m_skinTexId(-1)
    , m_eyesTexId(-1)
{
}

RenderHerobrine::~RenderHerobrine()
{
    delete m_eyeModel;
    m_eyeModel = nullptr;
    if (renderManager != nullptr && renderManager->renderEngine != nullptr)
    {
        if (m_skinTexId >= 0)
            renderManager->renderEngine->deleteTexture(m_skinTexId);
        if (m_eyesTexId >= 0)
            renderManager->renderEngine->deleteTexture(m_eyesTexId);
    }
    m_skinTexId = -1;
    m_eyesTexId = -1;
}

void RenderHerobrine::ensureTextures(RenderEngine *engine)
{
    if (engine == nullptr || (m_skinTexId >= 0 && m_eyesTexId >= 0))
        return;

    std::vector<int_t> pixels;
    int_t w = 64, h = 32;
    if (!engine->readTextureImageDataWithSize("/mob/char.png", pixels, w, h) ||
        w < 64 || h < 32 || pixels.size() < static_cast<size_t>(w * h))
    {
        w = 64;
        h = 32;
        buildFallbackSteveWithWhiteEyes(pixels, w, h);
    }

    // Classic Steve face is at u=[8..16), v=[8..16) in 64x32 coordinates;
    // Left eye: (9..10, 12), Right eye: (13..14, 12) (2x1 px each on 64x32 skin).
    const int_t sx = std::max<int_t>(1, w / 64);
    const int_t sy = std::max<int_t>(1, h / 32);
    const int_t pureWhite = static_cast<int_t>(0xFFFFFFFFu);

    // Overwrite both eye rectangles on the Steve skin with pure glowing white
    fillRect(pixels, w, h, 9 * sx, 12 * sy, 2 * sx, 1 * sy, pureWhite);
    fillRect(pixels, w, h, 13 * sx, 12 * sy, 2 * sx, 1 * sy, pureWhite);

    auto argbToRgba = [](const std::vector<int_t> &src, std::vector<unsigned char> &dst)
    {
        dst.resize(src.size() * 4);
        for (size_t i = 0; i < src.size(); ++i)
        {
            const uint32_t argb = static_cast<uint32_t>(src[i]);
            dst[i * 4 + 0] = static_cast<unsigned char>((argb >> 16) & 0xFF);
            dst[i * 4 + 1] = static_cast<unsigned char>((argb >> 8) & 0xFF);
            dst[i * 4 + 2] = static_cast<unsigned char>(argb & 0xFF);
            dst[i * 4 + 3] = static_cast<unsigned char>((argb >> 24) & 0xFF);
        }
    };

    std::vector<unsigned char> skinRgba;
    argbToRgba(pixels, skinRgba);
    BufferedImage skinImg(w, h);
    skinImg.setRGB(0, 0, w, h, skinRgba.data());
    m_skinTexId = engine->allocateAndSetupTexture(&skinImg);

    // Emissive eyes-only mask texture (fully transparent except the 2 white eyes)
    std::vector<int_t> eyePixels(static_cast<size_t>(w * h), 0);
    fillRect(eyePixels, w, h, 9 * sx, 12 * sy, 2 * sx, 1 * sy, pureWhite);
    fillRect(eyePixels, w, h, 13 * sx, 12 * sy, 2 * sx, 1 * sy, pureWhite);

    std::vector<unsigned char> eyeRgba;
    argbToRgba(eyePixels, eyeRgba);
    BufferedImage eyeImg(w, h);
    eyeImg.setRGB(0, 0, w, h, eyeRgba.data());
    m_eyesTexId = engine->allocateAndSetupTexture(&eyeImg);
}

void RenderHerobrine::renderModel(EntityLiving *entityliving, float f, float f1, float f2, float f3, float f4, float f5)
{
    if (renderManager == nullptr || renderManager->renderEngine == nullptr)
        return;

    ensureTextures(renderManager->renderEngine);
    if (m_skinTexId >= 0)
        renderManager->renderEngine->bindTexture(m_skinTexId);
    else
        loadTexture("/mob/char.png");

    mainModel->render(entityliving, f, f1, f2, f3, f4, f5);

    // Fullbright emissive white-eyes pass so Herobrine's eyes glow in the dark!
    if (m_eyesTexId >= 0 && m_eyeModel != nullptr)
    {
        renderManager->renderEngine->bindTexture(m_eyesTexId);
        renderDisable(RenderCapability::Lighting);
        renderEnable(RenderCapability::Blend);
        renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
        OpenGlHelper::setLightmapTextureCoords(OpenGlHelper::lightmapTexUnit, 240.0f, 240.0f);
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        m_eyeModel->render(f, f1, f2, f3, f4, f5);
        renderDisable(RenderCapability::Blend);
        renderEnable(RenderCapability::Lighting);
    }
}
