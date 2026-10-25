#include "RenderTntMinecart.h"
#include "EntityMinecartTNT.h"
#include "Block.h"
#include "RenderBlocks.h"
#include "platform/RenderAPI.h"

RenderTntMinecart::RenderTntMinecart()
    : RenderMinecart()
{
}

void RenderTntMinecart::renderBlockInMinecart(EntityMinecart* entityMinecart, float partialTicks, Block* block, int_t metadata)
{
    EntityMinecartTNT *cartTNT = dynamic_cast<EntityMinecartTNT*>(entityMinecart);
    int_t fuse = cartTNT != nullptr ? cartTNT->func_94104_d() : -1;
    if (fuse > -1 && (float)fuse - partialTicks + 1.0f < 10.0f)
    {
        float f2 = 1.0f - ((float)fuse - partialTicks + 1.0f) / 10.0f;
        if (f2 < 0.0f) f2 = 0.0f;
        if (f2 > 1.0f) f2 = 1.0f;
        f2 *= f2;
        f2 *= f2;
        float f3 = 1.0f + f2 * 0.3f;
        renderScale(f3, f3, f3);
    }

    RenderMinecart::renderBlockInMinecart(entityMinecart, partialTicks, block, metadata);

    if (fuse > -1 && fuse / 5 % 2 == 0)
    {
        renderDisable(RenderCapability::Texture2D);
        renderDisable(RenderCapability::Lighting);
        renderEnable(RenderCapability::Blend);
        renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::DstAlpha);
        renderColor4f(1.0f, 1.0f, 1.0f, (1.0f - ((float)fuse - partialTicks + 1.0f) / 100.0f) * 0.8f);
        renderPushMatrix();
        renderBlocks->renderBlockAsItem(Block::tnt, 0, 1.0f);
        renderPopMatrix();
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        renderDisable(RenderCapability::Blend);
        renderEnable(RenderCapability::Lighting);
        renderEnable(RenderCapability::Texture2D);
    }
}
