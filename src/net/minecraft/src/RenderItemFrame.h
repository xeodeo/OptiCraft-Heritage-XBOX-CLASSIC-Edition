#pragma once

#include "Render.h"
#include "RenderBlocks.h"

class EntityItemFrame;

// net.minecraft.src.RenderItemFrame
class RenderItemFrame : public Render
{
public:
    RenderItemFrame();
    ~RenderItemFrame() override = default;

    void func_82404_a(EntityItemFrame *itemFrame, double x, double y, double z, float yaw, float partialTicks);
    void doRender(Entity *entity, double x, double y, double z, float yaw, float partialTicks) override;

private:
    void renderFrameItemAsBlock(EntityItemFrame *itemFrame);
    void func_82402_b(EntityItemFrame *itemFrame);

    RenderBlocks renderBlocksInstance;
};
