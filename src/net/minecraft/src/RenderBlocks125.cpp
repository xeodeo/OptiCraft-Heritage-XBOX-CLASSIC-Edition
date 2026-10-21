#include "RenderBlocks.h"
#include "BlockTripWire.h"
#include "MathHelper.h"

#include <utility>

#include "Block.h"
#include "Config.h"
#include "ConnectedTextures.h"
#include "CustomColorizer.h"
#include "BlockBrewingStand.h"
#include "BlockCauldron.h"
#include "BlockDirectional.h"
#include "BlockDragonEgg.h"
#include "BlockEndPortalFrame.h"
#include "BlockFenceGate.h"
#include "BlockWall.h"
#include "BlockFlowerPot.h"
#include "BlockDeadBush.h"
#include "BlockTallGrass.h"
#include "BlockCocoa.h"
#include "BlockAnvil.h"
#include "BlockBeacon.h"
#include "Texture152Atlas.h"
#include "BlockLilyPad.h"
#include "BlockPane.h"
#include "BlockStem.h"
#include "EntityRenderer.h"
#include "IBlockAccess.h"
#include "Tessellator.h"
#include "java/Arithmetic.h"

namespace
{
void applyAnaglyph(float &red, float &green, float &blue)
{
    if (!EntityRenderer::anaglyphEnabled)
        return;

    const float convertedRed = (red * 30.0f + green * 59.0f + blue * 11.0f) / 100.0f;
    const float convertedGreen = (red * 30.0f + green * 70.0f) / 100.0f;
    const float convertedBlue = (red * 30.0f + blue * 70.0f) / 100.0f;
    red = convertedRed;
    green = convertedGreen;
    blue = convertedBlue;
}

void setBlockColor(Tessellator &tessellator, Block *block, IBlockAccess *access,
                   int_t x, int_t y, int_t z)
{
    const int_t color = block->colorMultiplier(access, x, y, z);
    float red = static_cast<float>((color >> 16) & 255) / 255.0f;
    float green = static_cast<float>((color >> 8) & 255) / 255.0f;
    float blue = static_cast<float>(color & 255) / 255.0f;
    applyAnaglyph(red, green, blue);
    tessellator.setColorOpaque_F(red, green, blue);
}
}

bool RenderBlocks::renderBlockStem(Block *block, int_t x, int_t y, int_t z)
{
    BlockStem *stem = static_cast<BlockStem *>(block);
    Tessellator &tessellator = Tessellator::instance;
    tessellator.setBrightness(stem->getMixedBrightnessForBlock(blockAccess, x, y, z));
    int_t stemColor = CustomColorizer::getStemColorMultiplier(stem, blockAccess, x, y, z);
    float stemRed = static_cast<float>((stemColor >> 16) & 255) / 255.0f;
    float stemGreen = static_cast<float>((stemColor >> 8) & 255) / 255.0f;
    float stemBlue = static_cast<float>(stemColor & 255) / 255.0f;
    applyAnaglyph(stemRed, stemGreen, stemBlue);
    tessellator.setColorOpaque_F(stemRed, stemGreen, stemBlue);
    stem->setBlockBoundsBasedOnState(blockAccess, x, y, z);

    const int_t metadata = accessGetBlockMetadata(x, y, z);
    const int_t direction = stem->getStateForNeighbor(blockAccess, x, y, z);
    if (direction < 0)
        renderBlockStemSmall(stem, metadata, stem->maxY, x, y, z);
    else
    {
        renderBlockStemSmall(stem, metadata, 0.5, x, y, z);
        renderBlockStemBig(stem, metadata, direction, stem->maxY, x, y, z);
    }
    return true;
}

void RenderBlocks::renderBlockStemSmall(Block *block, int_t metadata, double height,
                                        double x, double y, double z)
{
    Tessellator &tessellator = Tessellator::instance;
    int_t texture = block->getBlockTextureFromSideAndMetadata(0, metadata);
    if (overrideBlockTexture >= 0)
        texture = overrideBlockTexture;

    const int_t textureX = (texture & 15) << 4;
    const int_t textureY = texture & 240;
    const tess_coord_t renderHeight = static_cast<tess_coord_t>(height);
    const tess_coord_t renderX = static_cast<tess_coord_t>(x);
    const tess_coord_t renderY = static_cast<tess_coord_t>(y);
    const tess_coord_t renderZ = static_cast<tess_coord_t>(z);
    const tess_coord_t u0 = static_cast<tess_coord_t>(textureX) / 256.0f;
    const tess_coord_t u1 = (static_cast<tess_coord_t>(textureX) + static_cast<tess_coord_t>(15.99)) / 256.0f;
    const tess_coord_t v0 = static_cast<tess_coord_t>(textureY) / 256.0f;
    const tess_coord_t v1 = (static_cast<tess_coord_t>(textureY) + static_cast<tess_coord_t>(15.99) * renderHeight) / 256.0f;
    const tess_coord_t x0 = renderX + 0.5f - static_cast<tess_coord_t>(0.45);
    const tess_coord_t x1 = renderX + 0.5f + static_cast<tess_coord_t>(0.45);
    const tess_coord_t z0 = renderZ + 0.5f - static_cast<tess_coord_t>(0.45);
    const tess_coord_t z1 = renderZ + 0.5f + static_cast<tess_coord_t>(0.45);

    tessellator.addVertexWithUV(x0, renderY + renderHeight, z0, u0, v0);
    tessellator.addVertexWithUV(x0, renderY, z0, u0, v1);
    tessellator.addVertexWithUV(x1, renderY, z1, u1, v1);
    tessellator.addVertexWithUV(x1, renderY + renderHeight, z1, u1, v0);
    tessellator.addVertexWithUV(x1, renderY + renderHeight, z1, u0, v0);
    tessellator.addVertexWithUV(x1, renderY, z1, u0, v1);
    tessellator.addVertexWithUV(x0, renderY, z0, u1, v1);
    tessellator.addVertexWithUV(x0, renderY + renderHeight, z0, u1, v0);
    tessellator.addVertexWithUV(x0, renderY + renderHeight, z1, u0, v0);
    tessellator.addVertexWithUV(x0, renderY, z1, u0, v1);
    tessellator.addVertexWithUV(x1, renderY, z0, u1, v1);
    tessellator.addVertexWithUV(x1, renderY + renderHeight, z0, u1, v0);
    tessellator.addVertexWithUV(x1, renderY + renderHeight, z0, u0, v0);
    tessellator.addVertexWithUV(x1, renderY, z0, u0, v1);
    tessellator.addVertexWithUV(x0, renderY, z1, u1, v1);
    tessellator.addVertexWithUV(x0, renderY + renderHeight, z1, u1, v0);
}

void RenderBlocks::renderBlockStemBig(Block *block, int_t metadata, int_t direction,
                                      double height, double x, double y, double z)
{
    Tessellator &tessellator = Tessellator::instance;
    int_t texture = block->getBlockTextureFromSideAndMetadata(0, metadata) + 16;
    if (overrideBlockTexture >= 0)
        texture = overrideBlockTexture;

    const int_t textureX = (texture & 15) << 4;
    const int_t textureY = texture & 240;
    const tess_coord_t renderHeight = static_cast<tess_coord_t>(height);
    const tess_coord_t renderX = static_cast<tess_coord_t>(x);
    const tess_coord_t renderY = static_cast<tess_coord_t>(y);
    const tess_coord_t renderZ = static_cast<tess_coord_t>(z);
    tess_coord_t u0 = static_cast<tess_coord_t>(textureX) / 256.0f;
    tess_coord_t u1 = (static_cast<tess_coord_t>(textureX) + static_cast<tess_coord_t>(15.99)) / 256.0f;
    const tess_coord_t v0 = static_cast<tess_coord_t>(textureY) / 256.0f;
    const tess_coord_t v1 = (static_cast<tess_coord_t>(textureY) + static_cast<tess_coord_t>(15.99) * renderHeight) / 256.0f;
    const tess_coord_t x0 = renderX;
    const tess_coord_t x1 = renderX + 1.0f;
    const tess_coord_t z0 = renderZ;
    const tess_coord_t z1 = renderZ + 1.0f;
    const tess_coord_t centerX = renderX + 0.5f;
    const tess_coord_t centerZ = renderZ + 0.5f;

    if (((direction + 1) / 2) % 2 == 1)
        std::swap(u0, u1);

    if (direction < 2)
    {
        tessellator.addVertexWithUV(x0, renderY + renderHeight, centerZ, u0, v0);
        tessellator.addVertexWithUV(x0, renderY, centerZ, u0, v1);
        tessellator.addVertexWithUV(x1, renderY, centerZ, u1, v1);
        tessellator.addVertexWithUV(x1, renderY + renderHeight, centerZ, u1, v0);
        tessellator.addVertexWithUV(x1, renderY + renderHeight, centerZ, u1, v0);
        tessellator.addVertexWithUV(x1, renderY, centerZ, u1, v1);
        tessellator.addVertexWithUV(x0, renderY, centerZ, u0, v1);
        tessellator.addVertexWithUV(x0, renderY + renderHeight, centerZ, u0, v0);
    }
    else
    {
        tessellator.addVertexWithUV(centerX, renderY + renderHeight, z1, u0, v0);
        tessellator.addVertexWithUV(centerX, renderY, z1, u0, v1);
        tessellator.addVertexWithUV(centerX, renderY, z0, u1, v1);
        tessellator.addVertexWithUV(centerX, renderY + renderHeight, z0, u1, v0);
        tessellator.addVertexWithUV(centerX, renderY + renderHeight, z0, u1, v0);
        tessellator.addVertexWithUV(centerX, renderY, z0, u1, v1);
        tessellator.addVertexWithUV(centerX, renderY, z1, u0, v1);
        tessellator.addVertexWithUV(centerX, renderY + renderHeight, z1, u0, v0);
    }
}

bool RenderBlocks::renderBlockVine(Block *block, int_t x, int_t y, int_t z)
{
    Tessellator &tessellator = Tessellator::instance;
    int_t texture = block->getBlockTextureFromSide(0);
    if (overrideBlockTexture >= 0)
        texture = overrideBlockTexture;

    tessellator.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
    setBlockColor(tessellator, block, blockAccess, x, y, z);

    const int_t textureX = (texture & 15) << 4;
    const int_t textureY = texture & 240;
    const tess_coord_t u0 = static_cast<tess_coord_t>(textureX) / 256.0f;
    const tess_coord_t u1 = (static_cast<tess_coord_t>(textureX) + static_cast<tess_coord_t>(15.99)) / 256.0f;
    const tess_coord_t v0 = static_cast<tess_coord_t>(textureY) / 256.0f;
    const tess_coord_t v1 = (static_cast<tess_coord_t>(textureY) + static_cast<tess_coord_t>(15.99)) / 256.0f;
    const tess_coord_t inset = static_cast<tess_coord_t>(0.05);
    const int_t metadata = accessGetBlockMetadata(x, y, z);

    if ((metadata & 2) != 0)
    {
        tessellator.addVertexWithUV(x + inset, y + 1, z + 1, u0, v0);
        tessellator.addVertexWithUV(x + inset, y, z + 1, u0, v1);
        tessellator.addVertexWithUV(x + inset, y, z, u1, v1);
        tessellator.addVertexWithUV(x + inset, y + 1, z, u1, v0);
        tessellator.addVertexWithUV(x + inset, y + 1, z, u1, v0);
        tessellator.addVertexWithUV(x + inset, y, z, u1, v1);
        tessellator.addVertexWithUV(x + inset, y, z + 1, u0, v1);
        tessellator.addVertexWithUV(x + inset, y + 1, z + 1, u0, v0);
    }
    if ((metadata & 8) != 0)
    {
        tessellator.addVertexWithUV(x + 1 - inset, y, z + 1, u1, v1);
        tessellator.addVertexWithUV(x + 1 - inset, y + 1, z + 1, u1, v0);
        tessellator.addVertexWithUV(x + 1 - inset, y + 1, z, u0, v0);
        tessellator.addVertexWithUV(x + 1 - inset, y, z, u0, v1);
        tessellator.addVertexWithUV(x + 1 - inset, y, z, u0, v1);
        tessellator.addVertexWithUV(x + 1 - inset, y + 1, z, u0, v0);
        tessellator.addVertexWithUV(x + 1 - inset, y + 1, z + 1, u1, v0);
        tessellator.addVertexWithUV(x + 1 - inset, y, z + 1, u1, v1);
    }
    if ((metadata & 4) != 0)
    {
        tessellator.addVertexWithUV(x + 1, y, z + inset, u1, v1);
        tessellator.addVertexWithUV(x + 1, y + 1, z + inset, u1, v0);
        tessellator.addVertexWithUV(x, y + 1, z + inset, u0, v0);
        tessellator.addVertexWithUV(x, y, z + inset, u0, v1);
        tessellator.addVertexWithUV(x, y, z + inset, u0, v1);
        tessellator.addVertexWithUV(x, y + 1, z + inset, u0, v0);
        tessellator.addVertexWithUV(x + 1, y + 1, z + inset, u1, v0);
        tessellator.addVertexWithUV(x + 1, y, z + inset, u1, v1);
    }
    if ((metadata & 1) != 0)
    {
        tessellator.addVertexWithUV(x + 1, y + 1, z + 1 - inset, u0, v0);
        tessellator.addVertexWithUV(x + 1, y, z + 1 - inset, u0, v1);
        tessellator.addVertexWithUV(x, y, z + 1 - inset, u1, v1);
        tessellator.addVertexWithUV(x, y + 1, z + 1 - inset, u1, v0);
        tessellator.addVertexWithUV(x, y + 1, z + 1 - inset, u1, v0);
        tessellator.addVertexWithUV(x, y, z + 1 - inset, u1, v1);
        tessellator.addVertexWithUV(x + 1, y, z + 1 - inset, u0, v1);
        tessellator.addVertexWithUV(x + 1, y + 1, z + 1 - inset, u0, v0);
    }
    if (accessIsBlockNormalCube(x, y + 1, z))
    {
        tessellator.addVertexWithUV(x + 1, y + 1 - inset, z, u0, v0);
        tessellator.addVertexWithUV(x + 1, y + 1 - inset, z + 1, u0, v1);
        tessellator.addVertexWithUV(x, y + 1 - inset, z + 1, u1, v1);
        tessellator.addVertexWithUV(x, y + 1 - inset, z, u1, v0);
    }
    renderBetterSnow(x, y, z);
    return true;
}

bool RenderBlocks::renderBlockFenceGate(Block *block, int_t x, int_t y, int_t z)
{
    BlockFenceGate *gate = static_cast<BlockFenceGate *>(block);
    const int_t metadata = accessGetBlockMetadata(x, y, z);
    const bool open = BlockFenceGate::isFenceGateOpen(metadata);
    const int_t direction = BlockDirectional::getDirection(metadata);
    float yGateOffset = 0.0f;
    if (Block::cobblestoneWall != nullptr &&
        (((direction == 2 || direction == 0) &&
          accessGetBlockId(x - 1, y, z) == Block::cobblestoneWall->blockID &&
          accessGetBlockId(x + 1, y, z) == Block::cobblestoneWall->blockID) ||
         ((direction == 3 || direction == 1) &&
          accessGetBlockId(x, y, z - 1) == Block::cobblestoneWall->blockID &&
          accessGetBlockId(x, y, z + 1) == Block::cobblestoneWall->blockID)))
    {
        yGateOffset = -0.1875f;
    }
    float minA, maxA, minB, maxB;

    if (direction != 3 && direction != 1)
    {
        minA = 0.0f; maxA = 2.0f / 16.0f; minB = 7.0f / 16.0f; maxB = 9.0f / 16.0f;
        gate->setBlockBounds(minA, 5.0f / 16.0f + yGateOffset, minB, maxA, 1.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
        minA = 14.0f / 16.0f; maxA = 1.0f;
        gate->setBlockBounds(minA, 5.0f / 16.0f + yGateOffset, minB, maxA, 1.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
    }
    else
    {
        minA = 7.0f / 16.0f; maxA = 9.0f / 16.0f; minB = 0.0f; maxB = 2.0f / 16.0f;
        gate->setBlockBounds(minA, 5.0f / 16.0f + yGateOffset, minB, maxA, 1.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
        minB = 14.0f / 16.0f; maxB = 1.0f;
        gate->setBlockBounds(minA, 5.0f / 16.0f + yGateOffset, minB, maxA, 1.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
    }

    if (!open)
    {
        if (direction != 3 && direction != 1)
        {
            minA = 6.0f / 16.0f; maxA = 0.5f; minB = 7.0f / 16.0f; maxB = 9.0f / 16.0f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            minA = 0.5f; maxA = 10.0f / 16.0f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            minA = 10.0f / 16.0f; maxA = 14.0f / 16.0f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 9.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            gate->setBlockBounds(minA, 12.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            minA = 2.0f / 16.0f; maxA = 6.0f / 16.0f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 9.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            gate->setBlockBounds(minA, 12.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
        }
        else
        {
            minA = 7.0f / 16.0f; maxA = 9.0f / 16.0f; minB = 6.0f / 16.0f; maxB = 0.5f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            minB = 0.5f; maxB = 10.0f / 16.0f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            minB = 10.0f / 16.0f; maxB = 14.0f / 16.0f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 9.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            gate->setBlockBounds(minA, 12.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            minB = 2.0f / 16.0f; maxB = 6.0f / 16.0f;
            gate->setBlockBounds(minA, 6.0f / 16.0f + yGateOffset, minB, maxA, 9.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
            gate->setBlockBounds(minA, 12.0f / 16.0f + yGateOffset, minB, maxA, 15.0f / 16.0f + yGateOffset, maxB); renderStandardBlock(gate, x, y, z);
        }
    }
    else if (direction == 3)
    {
        gate->setBlockBounds(13.0f/16,6.0f/16 + yGateOffset,0,15.0f/16 + yGateOffset,15.0f/16,2.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(13.0f/16,6.0f/16 + yGateOffset,14.0f/16,15.0f/16 + yGateOffset,15.0f/16,1); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(9.0f/16,6.0f/16 + yGateOffset,0,13.0f/16,9.0f/16 + yGateOffset,2.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(9.0f/16,6.0f/16 + yGateOffset,14.0f/16,13.0f/16,9.0f/16 + yGateOffset,1); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(9.0f/16,12.0f/16 + yGateOffset,0,13.0f/16,15.0f/16 + yGateOffset,2.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(9.0f/16,12.0f/16 + yGateOffset,14.0f/16,13.0f/16,15.0f/16 + yGateOffset,1); renderStandardBlock(gate,x,y,z);
    }
    else if (direction == 1)
    {
        gate->setBlockBounds(1.0f/16,6.0f/16 + yGateOffset,0,3.0f/16,15.0f/16 + yGateOffset,2.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(1.0f/16,6.0f/16 + yGateOffset,14.0f/16,3.0f/16,15.0f/16 + yGateOffset,1); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(3.0f/16,6.0f/16 + yGateOffset,0,7.0f/16,9.0f/16 + yGateOffset,2.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(3.0f/16,6.0f/16 + yGateOffset,14.0f/16,7.0f/16,9.0f/16 + yGateOffset,1); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(3.0f/16,12.0f/16 + yGateOffset,0,7.0f/16,15.0f/16 + yGateOffset,2.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(3.0f/16,12.0f/16 + yGateOffset,14.0f/16,7.0f/16,15.0f/16 + yGateOffset,1); renderStandardBlock(gate,x,y,z);
    }
    else if (direction == 0)
    {
        gate->setBlockBounds(0,6.0f/16 + yGateOffset,13.0f/16,2.0f/16,15.0f/16 + yGateOffset,15.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(14.0f/16,6.0f/16 + yGateOffset,13.0f/16,1,15.0f/16 + yGateOffset,15.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(0,6.0f/16 + yGateOffset,9.0f/16 + yGateOffset,2.0f/16,9.0f/16 + yGateOffset,13.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(14.0f/16,6.0f/16 + yGateOffset,9.0f/16 + yGateOffset,1,9.0f/16 + yGateOffset,13.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(0,12.0f/16 + yGateOffset,9.0f/16 + yGateOffset,2.0f/16,15.0f/16 + yGateOffset,13.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(14.0f/16,12.0f/16 + yGateOffset,9.0f/16 + yGateOffset,1,15.0f/16 + yGateOffset,13.0f/16); renderStandardBlock(gate,x,y,z);
    }
    else if (direction == 2)
    {
        gate->setBlockBounds(0,6.0f/16 + yGateOffset,1.0f/16,2.0f/16,15.0f/16 + yGateOffset,3.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(14.0f/16,6.0f/16 + yGateOffset,1.0f/16,1,15.0f/16 + yGateOffset,3.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(0,6.0f/16 + yGateOffset,3.0f/16,2.0f/16,9.0f/16 + yGateOffset,7.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(14.0f/16,6.0f/16 + yGateOffset,3.0f/16,1,9.0f/16 + yGateOffset,7.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(0,12.0f/16 + yGateOffset,3.0f/16,2.0f/16,15.0f/16 + yGateOffset,7.0f/16); renderStandardBlock(gate,x,y,z);
        gate->setBlockBounds(14.0f/16,12.0f/16 + yGateOffset,3.0f/16,1,15.0f/16 + yGateOffset,7.0f/16); renderStandardBlock(gate,x,y,z);
    }

    gate->setBlockBounds(0, 0, 0, 1, 1, 1);
    return true;
}

bool RenderBlocks::renderBlockLilyPad(Block *block, int_t x, int_t y, int_t z)
{
    Tessellator &tessellator = Tessellator::instance;
    int_t texture = block->blockIndexInTexture;
    if (overrideBlockTexture >= 0)
        texture = overrideBlockTexture;

    const int_t textureX = (texture & 15) << 4;
    const int_t textureY = texture & 240;
    const float yOffset = 0.015625f;
    const tess_coord_t u0 = static_cast<tess_coord_t>(textureX) / 256.0f;
    const tess_coord_t u1 = (static_cast<tess_coord_t>(textureX) + static_cast<tess_coord_t>(15.99)) / 256.0f;
    const tess_coord_t v0 = static_cast<tess_coord_t>(textureY) / 256.0f;
    const tess_coord_t v1 = (static_cast<tess_coord_t>(textureY) + static_cast<tess_coord_t>(15.99)) / 256.0f;

    const int_t xHash = JavaArithmetic::intFromBits(static_cast<uint_t>(x) * UINT32_C(3129871));
    const long_t zHash = JavaArithmetic::longFromBits(static_cast<ulong_t>(static_cast<long_t>(z)) * UINT64_C(116129781));
    const ulong_t initialBits = static_cast<ulong_t>(static_cast<long_t>(xHash)) ^
                                static_cast<ulong_t>(zHash) ^
                                static_cast<ulong_t>(static_cast<long_t>(y));
    long_t seed = JavaArithmetic::longFromBits(initialBits);
    const ulong_t seedBits = static_cast<ulong_t>(seed);
    seed = JavaArithmetic::longFromBits(seedBits * seedBits * 42317861ULL + seedBits * 11ULL);
    const int_t rotation = static_cast<int_t>((static_cast<ulong_t>(seed) >> 16) & 3ULL);

    tessellator.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
    const float centerX = static_cast<float>(x) + 0.5f;
    const float centerZ = static_cast<float>(z) + 0.5f;
    const float dx = static_cast<float>(rotation & 1) * 0.5f * static_cast<float>(1 - (rotation / 2 % 2) * 2);
    const int_t nextRotation = rotation + 1;
    const float dz = static_cast<float>(nextRotation & 1) * 0.5f * static_cast<float>(1 - (nextRotation / 2 % 2) * 2);

    tessellator.setColorOpaque_I(CustomColorizer::getLilypadColor());
    tessellator.addVertexWithUV(centerX + dx - dz, y + yOffset, centerZ + dx + dz, u0, v0);
    tessellator.addVertexWithUV(centerX + dx + dz, y + yOffset, centerZ - dx + dz, u1, v0);
    tessellator.addVertexWithUV(centerX - dx + dz, y + yOffset, centerZ - dx - dz, u1, v1);
    tessellator.addVertexWithUV(centerX - dx - dz, y + yOffset, centerZ + dx - dz, u0, v1);
    tessellator.setColorOpaque_I((CustomColorizer::getLilypadColor() & 16711422) >> 1);
    tessellator.addVertexWithUV(centerX - dx - dz, y + yOffset, centerZ + dx - dz, u0, v1);
    tessellator.addVertexWithUV(centerX - dx + dz, y + yOffset, centerZ - dx - dz, u1, v1);
    tessellator.addVertexWithUV(centerX + dx + dz, y + yOffset, centerZ - dx + dz, u1, v0);
    tessellator.addVertexWithUV(centerX + dx - dz, y + yOffset, centerZ + dx + dz, u0, v0);
    return true;
}

bool RenderBlocks::renderBlockCauldron(Block *block, int_t x, int_t y, int_t z)
{
    renderStandardBlock(block, x, y, z);
    Tessellator &tessellator = Tessellator::instance;
    tessellator.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
    setBlockColor(tessellator, block, blockAccess, x, y, z);

    const float inset = 2.0f / 16.0f;
    renderSouthFace(block, x - 1.0f + inset, y, z, 154);
    renderNorthFace(block, x + 1.0f - inset, y, z, 154);
    renderWestFace(block, x, y, z - 1.0f + inset, 154);
    renderEastFace(block, x, y, z + 1.0f - inset, 154);
    renderTopFace(block, x, y - 1.0f + 0.25f, z, 139);
    renderBottomFace(block, x, y + 1.0f - 12.0f / 16.0f, z, 139);

    int_t metadata = accessGetBlockMetadata(x, y, z);
    if (metadata > 0)
    {
        if (metadata > 3)
            metadata = 3;
        renderTopFace(block, x, y - 1.0f + (6.0f + metadata * 3.0f) / 16.0f, z, 205);
    }
    return true;
}

bool RenderBlocks::renderBlockBrewingStand(Block *block, int_t x, int_t y, int_t z)
{
    block->setBlockBounds(7.0f/16,0,7.0f/16,9.0f/16,14.0f/16,9.0f/16); renderStandardBlock(block,x,y,z);
    overrideBlockTexture = 156;
    block->setBlockBounds(9.0f/16,0,5.0f/16,15.0f/16,2.0f/16,11.0f/16); renderStandardBlock(block,x,y,z);
    block->setBlockBounds(2.0f/16,0,1.0f/16,0.5f,2.0f/16,7.0f/16); renderStandardBlock(block,x,y,z);
    block->setBlockBounds(2.0f/16,0,9.0f/16,0.5f,2.0f/16,15.0f/16); renderStandardBlock(block,x,y,z);
    clearOverrideBlockTexture();

    Tessellator &tessellator = Tessellator::instance;
    tessellator.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
    setBlockColor(tessellator, block, blockAccess, x, y, z);
    int_t texture = block->getBlockTextureFromSideAndMetadata(0, 0);
    if (overrideBlockTexture >= 0)
        texture = overrideBlockTexture;
    const int_t textureX = (texture & 15) << 4;
    const int_t textureY = texture & 240;
    const tess_coord_t v0 = static_cast<tess_coord_t>(textureY) / 256.0f;
    const tess_coord_t v1 = (static_cast<tess_coord_t>(textureY) + static_cast<tess_coord_t>(15.99)) / 256.0f;
    const int_t metadata = accessGetBlockMetadata(x, y, z);
    // These are sin/cos(angle) * 0.5 for the three fixed brewing-stand
    // arms (90, 210 and 330 degrees). Avoiding libm here matters on the R5900:
    // this code runs while building terrain and the geometry never changes.
    static const tess_coord_t armOffsetX[3] = { 0.5f, -0.25f, -0.25f };
    static const tess_coord_t armOffsetZ[3] = { 0.0f, -static_cast<tess_coord_t>(0.4330127018922193), static_cast<tess_coord_t>(0.4330127018922193) };

    for (int_t arm = 0; arm < 3; ++arm)
    {
        tess_coord_t u0 = (static_cast<tess_coord_t>(textureX) + 8.0f) / 256.0f;
        tess_coord_t u1 = (static_cast<tess_coord_t>(textureX) + static_cast<tess_coord_t>(15.99)) / 256.0f;
        if ((metadata & (1 << arm)) != 0)
        {
            u0 = (static_cast<tess_coord_t>(textureX) + static_cast<tess_coord_t>(7.99)) / 256.0f;
            u1 = static_cast<tess_coord_t>(textureX) / 256.0f;
        }
        const tess_coord_t centerX = x + 0.5f;
        const tess_coord_t outerX = x + 0.5f + armOffsetX[arm];
        const tess_coord_t centerZ = z + 0.5f;
        const tess_coord_t outerZ = z + 0.5f + armOffsetZ[arm];
        tessellator.addVertexWithUV(centerX,y+1,centerZ,u0,v0);
        tessellator.addVertexWithUV(centerX,y,centerZ,u0,v1);
        tessellator.addVertexWithUV(outerX,y,outerZ,u1,v1);
        tessellator.addVertexWithUV(outerX,y+1,outerZ,u1,v0);
        tessellator.addVertexWithUV(outerX,y+1,outerZ,u1,v0);
        tessellator.addVertexWithUV(outerX,y,outerZ,u1,v1);
        tessellator.addVertexWithUV(centerX,y,centerZ,u0,v1);
        tessellator.addVertexWithUV(centerX,y+1,centerZ,u0,v0);
    }
    block->setBlockBoundsForItemRender();
    return true;
}

bool RenderBlocks::renderBlockEndPortalFrame(Block *block, int_t x, int_t y, int_t z)
{
    const int_t metadata = accessGetBlockMetadata(x, y, z);
    const int_t direction = metadata & 3;
    if (direction == 0) topFaceRotation = 3;
    else if (direction == 3) topFaceRotation = 1;
    else if (direction == 1) topFaceRotation = 2;

    block->setBlockBounds(0, 0, 0, 1, 13.0f / 16.0f, 1);
    renderStandardBlock(block, x, y, z);
    if (BlockEndPortalFrame::isEnderEyeInserted(metadata))
    {
        overrideBlockTexture = 174;
        block->setBlockBounds(0.25f, 13.0f/16, 0.25f, 12.0f/16, 1, 12.0f/16);
        renderStandardBlock(block, x, y, z);
        clearOverrideBlockTexture();
    }
    block->setBlockBoundsForItemRender();
    topFaceRotation = 0;
    return true;
}

bool RenderBlocks::renderBlockDragonEgg(Block *block, int_t x, int_t y, int_t z)
{
    int_t accumulatedHeight = 0;
    for (int_t layer = 0; layer < 8; ++layer)
    {
        int_t radiusUnits = 0;
        int_t heightUnits = 1;
        switch (layer)
        {
        case 0: radiusUnits = 2; break;
        case 1: radiusUnits = 3; break;
        case 2: radiusUnits = 4; break;
        case 3: radiusUnits = 5; heightUnits = 2; break;
        case 4: radiusUnits = 6; heightUnits = 3; break;
        case 5: radiusUnits = 7; heightUnits = 5; break;
        case 6: radiusUnits = 6; heightUnits = 2; break;
        case 7: radiusUnits = 3; break;
        }
        const float radius = static_cast<float>(radiusUnits) / 16.0f;
        const float top = 1.0f - static_cast<float>(accumulatedHeight) / 16.0f;
        const float bottom = 1.0f - static_cast<float>(accumulatedHeight + heightUnits) / 16.0f;
        accumulatedHeight += heightUnits;
        block->setBlockBounds(0.5f-radius,bottom,0.5f-radius,0.5f+radius,top,0.5f+radius);
        renderStandardBlock(block,x,y,z);
    }
    block->setBlockBounds(0,0,0,1,1,1);
    return true;
}

bool RenderBlocks::renderBlockPane(Block *block, int_t var2, int_t var3, int_t var4)
{
    BlockPane *var1 = static_cast<BlockPane *>(block);

    int_t var5 = blockAccess->getHeight();
    Tessellator &var6 = Tessellator::instance;
    bool connected = block == Block::thinGlass && ConnectedTextures::isConnectedGlassPanes();
    Tessellator *paneFront = &var6;
    var6.setBrightness(var1->getMixedBrightnessForBlock(blockAccess, var2, var3, var4));
    float var7 = 1.0F;
    int_t var8 = var1->colorMultiplier(blockAccess, var2, var3, var4);
    float var9 = (float)(var8 >> 16 & 255) / 255.0F;
    float var10 = (float)(var8 >> 8 & 255) / 255.0F;
    float var11 = (float)(var8 & 255) / 255.0F;
    if(EntityRenderer::anaglyphEnabled) {
    	float var12 = (var9 * 30.0F + var10 * 59.0F + var11 * 11.0F) / 100.0F;
    	float var13 = (var9 * 30.0F + var10 * 70.0F) / 100.0F;
    	float var14 = (var9 * 30.0F + var11 * 70.0F) / 100.0F;
    	var9 = var12;
    	var10 = var13;
    	var11 = var14;
    }

    var6.setColorOpaque_F(var7 * var9, var7 * var10, var7 * var11);
    bool var64 = false;
    bool var66 = false;
    int_t var65;
    int_t var67;
    int_t var68;
    if(overrideBlockTexture >= 0) {
    	var65 = overrideBlockTexture;
    	var67 = overrideBlockTexture;
		connected = false;
    } else {
    	var68 = accessGetBlockMetadata(var2, var3, var4);
    	var65 = var1->getBlockTextureFromSideAndMetadata(0, var68);
    	var67 = var1->getSideTextureIndex();
		if (connected)
		{
			const int_t ctmTexture = ConnectedTextures::getCtmTextureId();
			if (ctmTexture >= 0)
			{
				paneFront = var6.getSubTessellator(ctmTexture);
				var65 = 0;
			}
			else
			{
				connected = false;
			}
		}
    }

	int_t paneTextureX = var65;
	int_t paneTextureXReverse = var65;
	int_t paneTextureZ = var65;
	int_t paneTextureZReverse = var65;
	if (connected)
	{
		const int_t glassPaneId = Block::thinGlass->blockID;
		const bool linkXp = accessGetBlockId(var2 + 1, var3, var4) == glassPaneId;
		const bool linkXn = accessGetBlockId(var2 - 1, var3, var4) == glassPaneId;
		const bool linkYp = accessGetBlockId(var2, var3 + 1, var4) == glassPaneId;
		const bool linkYn = accessGetBlockId(var2, var3 - 1, var4) == glassPaneId;
		const bool linkZp = accessGetBlockId(var2, var3, var4 + 1) == glassPaneId;
		const bool linkZn = accessGetBlockId(var2, var3, var4 - 1) == glassPaneId;
		paneTextureX = ConnectedTextures::getGlassPaneTexture(linkXp, linkXn, linkYp, linkYn);
		paneTextureXReverse = ConnectedTextures::getReverseGlassPaneTexture(paneTextureX);
		paneTextureZ = ConnectedTextures::getGlassPaneTexture(linkZp, linkZn, linkYp, linkYn);
		paneTextureZReverse = ConnectedTextures::getReverseGlassPaneTexture(paneTextureZ);
	}
#if PLATFORM_PS2
	// The PS2 direct terrain path draws both windings of these coplanar pane
	// faces. Use the same atlas tile on the reverse winding; its U coordinates
	// are flipped below so every spatial corner samples the same texel as the
	// front face instead of overlaying a mirrored copy.
	paneTextureXReverse = paneTextureX;
	paneTextureZReverse = paneTextureZ;
#endif

    var68 = (paneTextureX & 15) << 4;
    int_t var15 = paneTextureX & 240;
    tess_coord_t var16 = (tess_coord_t)((float)var68 / 256.0F);
    tess_coord_t var18 = (tess_coord_t)(((float)var68 + 7.99F) / 256.0F);
    tess_coord_t var20 = (tess_coord_t)(((float)var68 + 15.99F) / 256.0F);
    tess_coord_t var22 = (tess_coord_t)((float)var15 / 256.0F);
    tess_coord_t var24 = (tess_coord_t)(((float)var15 + 15.99F) / 256.0F);
	int_t paneReverseU = (paneTextureXReverse & 15) << 4;
	int_t paneReverseV = paneTextureXReverse & 240;
	tess_coord_t paneXReverse0 = (tess_coord_t)((float)paneReverseU / 256.0F);
	tess_coord_t paneXReverseHalf = (tess_coord_t)(((float)paneReverseU + 7.99F) / 256.0F);
	tess_coord_t paneXReverse1 = (tess_coord_t)(((float)paneReverseU + 15.99F) / 256.0F);
	tess_coord_t paneXReverseV0 = (tess_coord_t)((float)paneReverseV / 256.0F);
	tess_coord_t paneXReverseV1 = (tess_coord_t)(((float)paneReverseV + 15.99F) / 256.0F);
#if PLATFORM_PS2
	paneXReverse0 = var20;
	paneXReverseHalf = var18;
	paneXReverse1 = var16;
	paneXReverseV0 = var22;
	paneXReverseV1 = var24;
#endif

	int_t paneZU = (paneTextureZ & 15) << 4;
	int_t paneZV = paneTextureZ & 240;
	tess_coord_t paneZ0 = (tess_coord_t)((float)paneZU / 256.0F);
	tess_coord_t paneZHalf = (tess_coord_t)(((float)paneZU + 7.99F) / 256.0F);
	tess_coord_t paneZ1 = (tess_coord_t)(((float)paneZU + 15.99F) / 256.0F);
	tess_coord_t paneZV0 = (tess_coord_t)((float)paneZV / 256.0F);
	tess_coord_t paneZV1 = (tess_coord_t)(((float)paneZV + 15.99F) / 256.0F);

	int_t paneZReverseU = (paneTextureZReverse & 15) << 4;
	int_t paneZReverseV = paneTextureZReverse & 240;
	tess_coord_t paneZReverse0 = (tess_coord_t)((float)paneZReverseU / 256.0F);
	tess_coord_t paneZReverseHalf = (tess_coord_t)(((float)paneZReverseU + 7.99F) / 256.0F);
	tess_coord_t paneZReverse1 = (tess_coord_t)(((float)paneZReverseU + 15.99F) / 256.0F);
	tess_coord_t paneZReverseV0 = (tess_coord_t)((float)paneZReverseV / 256.0F);
	tess_coord_t paneZReverseV1 = (tess_coord_t)(((float)paneZReverseV + 15.99F) / 256.0F);
#if PLATFORM_PS2
	paneZReverse0 = paneZ1;
	paneZReverseHalf = paneZHalf;
	paneZReverse1 = paneZ0;
	paneZReverseV0 = paneZV0;
	paneZReverseV1 = paneZV1;
#endif
    int_t var26 = (var67 & 15) << 4;
    int_t var27 = var67 & 240;
    tess_coord_t var28 = (tess_coord_t)((float)(var26 + 7) / 256.0F);
    tess_coord_t var30 = (tess_coord_t)(((float)var26 + 8.99F) / 256.0F);
    tess_coord_t var32 = (tess_coord_t)((float)var27 / 256.0F);
    tess_coord_t var34 = (tess_coord_t)((float)(var27 + 8) / 256.0F);
    tess_coord_t var36 = (tess_coord_t)(((float)var27 + 15.99F) / 256.0F);
    tess_coord_t var38 = (tess_coord_t)var2;
    tess_coord_t var40 = (tess_coord_t)var2 + 0.5f;
    tess_coord_t var42 = (tess_coord_t)(var2 + 1);
    tess_coord_t var44 = (tess_coord_t)var4;
    tess_coord_t var46 = (tess_coord_t)var4 + 0.5f;
    tess_coord_t var48 = (tess_coord_t)(var4 + 1);
    tess_coord_t var50 = (tess_coord_t)var2 + 0.5f - 1.0f / 16.0f;
    tess_coord_t var52 = (tess_coord_t)var2 + 0.5f + 1.0f / 16.0f;
    tess_coord_t var54 = (tess_coord_t)var4 + 0.5f - 1.0f / 16.0f;
    tess_coord_t var56 = (tess_coord_t)var4 + 0.5f + 1.0f / 16.0f;
    bool var58 = var1->canThisPaneConnectToThisBlockID(accessGetBlockId(var2, var3, var4 - 1));
    bool var59 = var1->canThisPaneConnectToThisBlockID(accessGetBlockId(var2, var3, var4 + 1));
    bool var60 = var1->canThisPaneConnectToThisBlockID(accessGetBlockId(var2 - 1, var3, var4));
    bool var61 = var1->canThisPaneConnectToThisBlockID(accessGetBlockId(var2 + 1, var3, var4));
    bool var62 = var1->shouldSideBeRendered(blockAccess, var2, var3 + 1, var4, 1);
    bool var63 = var1->shouldSideBeRendered(blockAccess, var2, var3 - 1, var4, 0);
    if((!var60 || !var61) && (var60 || var61 || var58 || var59)) {
    	if(var60 && !var61) {
	    		paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 1), var46, var16, var22);
	    		paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 0), var46, var16, var24);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, var18, var24);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, var18, var22);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, paneXReverseHalf, paneXReverseV0);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, paneXReverseHalf, paneXReverseV1);
	    		paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 0), var46, paneXReverse1, paneXReverseV1);
	    		paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 1), var46, paneXReverse1, paneXReverseV0);
    		if(!var59 && !var58) {
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var56, var28, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var56, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var54, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var54, var30, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var54, var28, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var54, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var56, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var56, var30, var32);
    		}

    		if(var62 || var3 < var5 - 1 && accessIsAirBlock(var2 - 1, var3 + 1, var4)) {
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    		}

    		if(var63 || var3 > 1 && accessIsAirBlock(var2 - 1, var3 - 1, var4)) {
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    		}
    	} else if(!var60 && var61) {
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, var18, var22);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, var18, var24);
	    		paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 0), var46, var20, var24);
	    		paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 1), var46, var20, var22);
	    		paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 1), var46, paneXReverse0, paneXReverseV0);
	    		paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 0), var46, paneXReverse0, paneXReverseV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, paneXReverseHalf, paneXReverseV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, paneXReverseHalf, paneXReverseV0);
    		if(!var59 && !var58) {
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var54, var28, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var54, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var56, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var56, var30, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var56, var28, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var56, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var54, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var54, var30, var32);
    		}

    		if(var62 || var3 < var5 - 1 && accessIsAirBlock(var2 + 1, var3 + 1, var4)) {
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		}

    		if(var63 || var3 > 1 && accessIsAirBlock(var2 + 1, var3 - 1, var4)) {
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		}
    	}
    } else {
	    	paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 1), var46, var16, var22);
	    	paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 0), var46, var16, var24);
	    	paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 0), var46, var20, var24);
	    	paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 1), var46, var20, var22);
	    	paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 1), var46, paneXReverse0, paneXReverseV0);
	    	paneFront->addVertexWithUV(var42, (tess_coord_t)(var3 + 0), var46, paneXReverse0, paneXReverseV1);
	    	paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 0), var46, paneXReverse1, paneXReverseV1);
	    	paneFront->addVertexWithUV(var38, (tess_coord_t)(var3 + 1), var46, paneXReverse1, paneXReverseV0);
    	if(var62) {
    		var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var36);
    		var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var32);
    		var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var36);
    		var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var36);
    		var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var32);
    		var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var36);
    	} else {
    		if(var3 < var5 - 1 && accessIsAirBlock(var2 - 1, var3 + 1, var4)) {
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    		}

    		if(var3 < var5 - 1 && accessIsAirBlock(var2 + 1, var3 + 1, var4)) {
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)(var3 + 1) + static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		}
    	}

    	if(var63) {
    		var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var36);
    		var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var32);
    		var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var36);
    		var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var36);
    		var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var32);
    		var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var36);
    	} else {
    		if(var3 > 1 && accessIsAirBlock(var2 - 1, var3 - 1, var4)) {
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var36);
    			var6.addVertexWithUV(var38, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var36);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    		}

    		if(var3 > 1 && accessIsAirBlock(var2 + 1, var3 - 1, var4)) {
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var32);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var32);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var56, var30, var34);
    			var6.addVertexWithUV(var40, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var34);
    			var6.addVertexWithUV(var42, (tess_coord_t)var3 - static_cast<tess_coord_t>(0.01), var54, var28, var32);
    		}
    	}
    }

    if((!var58 || !var59) && (var60 || var61 || var58 || var59)) {
    	if(var58 && !var59) {
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var44, paneZ0, paneZV0);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var44, paneZ0, paneZV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, paneZHalf, paneZV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, paneZHalf, paneZV0);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, paneZReverseHalf, paneZReverseV0);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, paneZReverseHalf, paneZReverseV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var44, paneZReverse1, paneZReverseV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var44, paneZReverse1, paneZReverseV0);
    		if(!var61 && !var60) {
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var28, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 0), var46, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 0), var46, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var30, var32);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var28, var32);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 0), var46, var28, var36);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 0), var46, var30, var36);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var30, var32);
    		}

    		if(var62 || var3 < var5 - 1 && accessIsAirBlock(var2, var3 + 1, var4 - 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var44, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var44, var28, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var44, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var44, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var28, var32);
    		}

    		if(var63 || var3 > 1 && accessIsAirBlock(var2, var3 - 1, var4 - 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var44, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var44, var28, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var44, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var44, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var28, var32);
    		}
    	} else if(!var58 && var59) {
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, paneZHalf, paneZV0);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, paneZHalf, paneZV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var48, paneZ1, paneZV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var48, paneZ1, paneZV0);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var48, paneZReverse0, paneZReverseV0);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var48, paneZReverse0, paneZReverseV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var46, paneZReverseHalf, paneZReverseV1);
	    		paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var46, paneZReverseHalf, paneZReverseV0);
    		if(!var61 && !var60) {
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var28, var32);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 0), var46, var28, var36);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 0), var46, var30, var36);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var28, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 0), var46, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 0), var46, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var30, var32);
    		}

    		if(var62 || var3 < var5 - 1 && accessIsAirBlock(var2, var3 + 1, var4 + 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var48, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var48, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var30, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var48, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var48, var30, var34);
    		}

    		if(var63 || var3 > 1 && accessIsAirBlock(var2, var3 - 1, var4 + 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var48, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var48, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var30, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var48, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var48, var30, var34);
    		}
    	}
    } else {
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var48, paneZReverse0, paneZReverseV0);
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var48, paneZReverse0, paneZReverseV1);
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var44, paneZReverse1, paneZReverseV1);
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var44, paneZReverse1, paneZReverseV0);
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var44, paneZ0, paneZV0);
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var44, paneZ0, paneZV1);
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 0), var48, paneZ1, paneZV1);
	    	paneFront->addVertexWithUV(var40, (tess_coord_t)(var3 + 1), var48, paneZ1, paneZV0);
    	if(var62) {
    		var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var48, var30, var36);
    		var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var44, var30, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var44, var28, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var48, var28, var36);
    		var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var44, var30, var36);
    		var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var48, var30, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var48, var28, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var44, var28, var36);
    	} else {
    		if(var3 < var5 - 1 && accessIsAirBlock(var2, var3 + 1, var4 - 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var44, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var44, var28, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var44, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var44, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var28, var32);
    		}

    		if(var3 < var5 - 1 && accessIsAirBlock(var2, var3 + 1, var4 + 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var48, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var48, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var30, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var48, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)(var3 + 1), var46, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var46, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)(var3 + 1), var48, var30, var34);
    		}
    	}

    	if(var63) {
    		var6.addVertexWithUV(var52, (tess_coord_t)var3, var48, var30, var36);
    		var6.addVertexWithUV(var52, (tess_coord_t)var3, var44, var30, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)var3, var44, var28, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)var3, var48, var28, var36);
    		var6.addVertexWithUV(var52, (tess_coord_t)var3, var44, var30, var36);
    		var6.addVertexWithUV(var52, (tess_coord_t)var3, var48, var30, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)var3, var48, var28, var32);
    		var6.addVertexWithUV(var50, (tess_coord_t)var3, var44, var28, var36);
    	} else {
    		if(var3 > 1 && accessIsAirBlock(var2, var3 - 1, var4 - 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var44, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var44, var28, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var30, var32);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var44, var30, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var44, var28, var34);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var28, var32);
    		}

    		if(var3 > 1 && accessIsAirBlock(var2, var3 - 1, var4 + 1)) {
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var48, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var48, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var30, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var48, var28, var34);
    			var6.addVertexWithUV(var50, (tess_coord_t)var3, var46, var28, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var46, var30, var36);
    			var6.addVertexWithUV(var52, (tess_coord_t)var3, var48, var30, var34);
    		}
    	}
    }

    return true;

}

void RenderBlocks::drawCrossedSquares(Block *block, int_t i, double d, double d1, double d2, float scale)
{
	Tessellator *tessellator = &Tessellator::instance;
	int_t j = block->getBlockTextureFromSideAndMetadata(0, i);
	if (overrideBlockTexture >= 0)
	{
		j = overrideBlockTexture;
	}
	int_t k = (j & 0xf) << 4;
	int_t l = j & 0xf0;
	tess_coord_t d3 = (float)k / 256.0f;
	tess_coord_t d4 = ((float)k + 15.99f) / 256.0f;
	tess_coord_t d5 = (float)l / 256.0f;
	tess_coord_t d6 = ((float)l + 15.99f) / 256.0f;
	const tess_coord_t x = (tess_coord_t)d;
	const tess_coord_t y = (tess_coord_t)d1;
	const tess_coord_t z = (tess_coord_t)d2;
	const tess_coord_t half = 0.5f;
	const tess_coord_t inset = (tess_coord_t)(0.45f * scale);
	tess_coord_t d7 = (x + half) - inset;
	tess_coord_t d8 = x + half + inset;
	tess_coord_t d9 = (z + half) - inset;
	tess_coord_t d10 = z + half + inset;
#if PLATFORM_PS2
	const tess_coord_t reverseLowU = d3;
	const tess_coord_t reverseHighU = d4;
#else
	const tess_coord_t reverseLowU = d4;
	const tess_coord_t reverseHighU = d3;
#endif
	tessellator->addVertexWithUV(d7, y + scale, d9, d3, d5);
	tessellator->addVertexWithUV(d7, y, d9, d3, d6);
	tessellator->addVertexWithUV(d8, y, d10, d4, d6);
	tessellator->addVertexWithUV(d8, y + scale, d10, d4, d5);
	tessellator->addVertexWithUV(d8, y + scale, d10, reverseHighU, d5);
	tessellator->addVertexWithUV(d8, y, d10, reverseHighU, d6);
	tessellator->addVertexWithUV(d7, y, d9, reverseLowU, d6);
	tessellator->addVertexWithUV(d7, y + scale, d9, reverseLowU, d5);
	tessellator->addVertexWithUV(d7, y + scale, d10, d3, d5);
	tessellator->addVertexWithUV(d7, y, d10, d3, d6);
	tessellator->addVertexWithUV(d8, y, d9, d4, d6);
	tessellator->addVertexWithUV(d8, y + scale, d9, d4, d5);
	tessellator->addVertexWithUV(d8, y + scale, d9, reverseHighU, d5);
	tessellator->addVertexWithUV(d8, y, d9, reverseHighU, d6);
	tessellator->addVertexWithUV(d7, y, d10, reverseLowU, d6);
	tessellator->addVertexWithUV(d7, y + scale, d10, reverseLowU, d5);
}

bool RenderBlocks::renderBlockWall(BlockWall *block, int_t x, int_t y, int_t z)
{
	bool west = block->canConnectWallTo(blockAccess, x - 1, y, z);
	bool east = block->canConnectWallTo(blockAccess, x + 1, y, z);
	bool north = block->canConnectWallTo(blockAccess, x, y, z - 1);
	bool south = block->canConnectWallTo(blockAccess, x, y, z + 1);
	bool northSouth = north && south && !west && !east;
	bool eastWest = !north && !south && west && east;
	bool aboveAir = accessIsAirBlock(x, y + 1, z);
	if ((northSouth || eastWest) && aboveAir)
	{
		if (northSouth)
		{
			block->setBlockBounds(0.3125, 0.0, 0.0, 0.6875, 0.8125, 1.0);
			renderStandardBlock(block, x, y, z);
		}
		else
		{
			block->setBlockBounds(0.0, 0.0, 0.3125, 1.0, 0.8125, 0.6875);
			renderStandardBlock(block, x, y, z);
		}
	}
	else
	{
		block->setBlockBounds(0.25, 0.0, 0.25, 0.75, 1.0, 0.75);
		renderStandardBlock(block, x, y, z);
		if (west)
		{
			block->setBlockBounds(0.0, 0.0, 0.3125, 0.25, 0.8125, 0.6875);
			renderStandardBlock(block, x, y, z);
		}
		if (east)
		{
			block->setBlockBounds(0.75, 0.0, 0.3125, 1.0, 0.8125, 0.6875);
			renderStandardBlock(block, x, y, z);
		}
		if (north)
		{
			block->setBlockBounds(0.3125, 0.0, 0.0, 0.6875, 0.8125, 0.25);
			renderStandardBlock(block, x, y, z);
		}
		if (south)
		{
			block->setBlockBounds(0.3125, 0.0, 0.75, 0.6875, 0.8125, 1.0);
			renderStandardBlock(block, x, y, z);
		}
	}
	block->setBlockBoundsBasedOnState(blockAccess, x, y, z);
	return true;
}

bool RenderBlocks::renderBlockFlowerpot(BlockFlowerPot *block, int_t x, int_t y, int_t z)
{
	renderStandardBlock(block, x, y, z);
	Tessellator &tessellator = Tessellator::instance;
	tessellator.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
	float f = 1.0f;
	int_t color = block->colorMultiplier(blockAccess, x, y, z);
	int_t tex = block->getBlockTextureFromSide(0);
	float r = static_cast<float>(color >> 16 & 0xff) / 255.0f;
	float g = static_cast<float>(color >> 8 & 0xff) / 255.0f;
	float b = static_cast<float>(color & 0xff) / 255.0f;
	tessellator.setColorOpaque_F(f * r, f * g, f * b);

	float inset = 0.1865f;
	renderSouthFace(block, static_cast<double>(static_cast<float>(x) - 0.5f + inset), static_cast<double>(y), static_cast<double>(z), tex);
	renderNorthFace(block, static_cast<double>(static_cast<float>(x) + 0.5f - inset), static_cast<double>(y), static_cast<double>(z), tex);
	renderWestFace(block, static_cast<double>(x), static_cast<double>(y), static_cast<double>(static_cast<float>(z) - 0.5f + inset), tex);
	renderEastFace(block, static_cast<double>(x), static_cast<double>(y), static_cast<double>(static_cast<float>(z) + 0.5f - inset), tex);
	int_t dirtTex = Block::dirt != nullptr ? Block::dirt->getBlockTextureFromSide(1) : 2;
	renderTopFace(block, static_cast<double>(x), static_cast<double>(static_cast<float>(y) - 0.5f + inset + 0.1875f), static_cast<double>(z), dirtTex);

	int_t meta = accessGetBlockMetadata(x, y, z);
	if (meta != 0)
	{
		Block *flower = nullptr;
		switch (meta)
		{
		case 1: flower = Block::plantRed; break;
		case 2: flower = Block::plantYellow; break;
		case 7: flower = Block::mushroomRed; break;
		case 8: flower = Block::mushroomBrown; break;
		default: break;
		}

		tessellator.addTranslation(0.0f, 4.0f / 16.0f, 0.0f);
		if (flower != nullptr)
		{
			renderBlockByRenderType(flower, x, y, z);
		}
		else if (meta == 9 && Block::cactus != nullptr)
		{
			renderAllFaces = true;
			float rCact = 0.125f;
			Block::cactus->setBlockBounds(0.5f - rCact, 0.0, 0.5f - rCact, 0.5f + rCact, 0.25, 0.5f + rCact);
			renderStandardBlock(Block::cactus, x, y, z);
			Block::cactus->setBlockBounds(0.5f - rCact, 0.25, 0.5f - rCact, 0.5f + rCact, 0.5, 0.5f + rCact);
			renderStandardBlock(Block::cactus, x, y, z);
			Block::cactus->setBlockBounds(0.5f - rCact, 0.5, 0.5f - rCact, 0.5f + rCact, 0.75, 0.5f + rCact);
			renderStandardBlock(Block::cactus, x, y, z);
			renderAllFaces = false;
			Block::cactus->setBlockBounds(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
		}
		else if (meta == 3 && Block::sapling != nullptr)
		{
			drawCrossedSquares(Block::sapling, 0, x, y, z, 0.75f);
		}
		else if (meta == 4 && Block::sapling != nullptr)
		{
			drawCrossedSquares(Block::sapling, 1, x, y, z, 0.75f);
		}
		else if (meta == 5 && Block::sapling != nullptr)
		{
			drawCrossedSquares(Block::sapling, 2, x, y, z, 0.75f);
		}
		else if (meta == 6 && Block::sapling != nullptr)
		{
			drawCrossedSquares(Block::sapling, 3, x, y, z, 0.75f);
		}
		else if (meta == 10 && Block::deadBush != nullptr)
		{
			drawCrossedSquares(Block::deadBush, 2, x, y, z, 0.75f);
		}
		else if (meta == 11 && Block::tallGrass != nullptr)
		{
			int_t c = Block::tallGrass->colorMultiplier(blockAccess, x, y, z);
			float cr = static_cast<float>(c >> 16 & 0xff) / 255.0f;
			float cg = static_cast<float>(c >> 8 & 0xff) / 255.0f;
			float cb = static_cast<float>(c & 0xff) / 255.0f;
			tessellator.setColorOpaque_F(f * cr, f * cg, f * cb);
			drawCrossedSquares(Block::tallGrass, 2, x, y, z, 0.75f);
		}
		tessellator.addTranslation(0.0f, -4.0f / 16.0f, 0.0f);
	}
	return true;
}

bool RenderBlocks::renderBlockCocoa(BlockCocoa *block, int_t x, int_t y, int_t z)
{
	Tessellator &tessellator = Tessellator::instance;
	tessellator.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
	tessellator.setColorOpaque_F(1.0f, 1.0f, 1.0f);
	int_t meta = accessGetBlockMetadata(x, y, z);
	int_t dir = BlockDirectional::getDirection(meta);
	int_t stage = BlockCocoa::func_72219_c(meta);
	int_t tex = BlockCocoa::getCocoaIconForStage(stage);
	float tileX = static_cast<float>((tex & 15) << 4);
	float tileY = static_cast<float>(tex & 240);
	auto interpU = [tileX](double t) -> tess_coord_t { return static_cast<tess_coord_t>((tileX + static_cast<float>(t)) / 256.0f); };
	auto interpV = [tileY](double t) -> tess_coord_t { return static_cast<tess_coord_t>((tileY + static_cast<float>(t)) / 256.0f); };

	int_t sizeW = 4 + stage * 2;
	int_t sizeH = 5 + stage * 2;
	tess_coord_t uMin = interpU(15.0 - static_cast<double>(sizeW));
	tess_coord_t uMax = interpU(15.0);
	tess_coord_t vMin = interpV(4.0);
	tess_coord_t vMax = interpV(4.0 + static_cast<double>(sizeH));
	double posX = 0.0;
	double posZ = 0.0;
	switch (dir)
	{
	case 0:
		posX = 8.0 - static_cast<double>(sizeW / 2);
		posZ = 15.0 - static_cast<double>(sizeW);
		break;
	case 1:
		posX = 1.0;
		posZ = 8.0 - static_cast<double>(sizeW / 2);
		break;
	case 2:
		posX = 8.0 - static_cast<double>(sizeW / 2);
		posZ = 1.0;
		break;
	case 3:
		posX = 15.0 - static_cast<double>(sizeW);
		posZ = 8.0 - static_cast<double>(sizeW / 2);
		break;
	}

	double minX = static_cast<double>(x) + posX / 16.0;
	double maxX = static_cast<double>(x) + (posX + static_cast<double>(sizeW)) / 16.0;
	double minY = static_cast<double>(y) + (12.0 - static_cast<double>(sizeH)) / 16.0;
	double maxY = static_cast<double>(y) + 0.75;
	double minZ = static_cast<double>(z) + posZ / 16.0;
	double maxZ = static_cast<double>(z) + (posZ + static_cast<double>(sizeW)) / 16.0;

	tessellator.addVertexWithUV(minX, minY, minZ, uMin, vMax);
	tessellator.addVertexWithUV(minX, minY, maxZ, uMax, vMax);
	tessellator.addVertexWithUV(minX, maxY, maxZ, uMax, vMin);
	tessellator.addVertexWithUV(minX, maxY, minZ, uMin, vMin);

	tessellator.addVertexWithUV(maxX, minY, maxZ, uMin, vMax);
	tessellator.addVertexWithUV(maxX, minY, minZ, uMax, vMax);
	tessellator.addVertexWithUV(maxX, maxY, minZ, uMax, vMin);
	tessellator.addVertexWithUV(maxX, maxY, maxZ, uMin, vMin);

	tessellator.addVertexWithUV(maxX, minY, minZ, uMin, vMax);
	tessellator.addVertexWithUV(minX, minY, minZ, uMax, vMax);
	tessellator.addVertexWithUV(minX, maxY, minZ, uMax, vMin);
	tessellator.addVertexWithUV(maxX, maxY, minZ, uMin, vMin);

	tessellator.addVertexWithUV(minX, minY, maxZ, uMin, vMax);
	tessellator.addVertexWithUV(maxX, minY, maxZ, uMax, vMax);
	tessellator.addVertexWithUV(maxX, maxY, maxZ, uMax, vMin);
	tessellator.addVertexWithUV(minX, maxY, maxZ, uMin, vMin);

	int_t topW = sizeW;
	if (stage >= 2) topW = sizeW - 1;
	uMin = interpU(0.0);
	uMax = interpU(static_cast<double>(topW));
	vMin = interpV(0.0);
	vMax = interpV(static_cast<double>(topW));

	tessellator.addVertexWithUV(minX, maxY, maxZ, uMin, vMax);
	tessellator.addVertexWithUV(maxX, maxY, maxZ, uMax, vMax);
	tessellator.addVertexWithUV(maxX, maxY, minZ, uMax, vMin);
	tessellator.addVertexWithUV(minX, maxY, minZ, uMin, vMin);

	tessellator.addVertexWithUV(minX, minY, minZ, uMin, vMin);
	tessellator.addVertexWithUV(maxX, minY, minZ, uMax, vMin);
	tessellator.addVertexWithUV(maxX, minY, maxZ, uMax, vMax);
	tessellator.addVertexWithUV(minX, minY, maxZ, uMin, vMax);

	uMin = interpU(12.0);
	uMax = interpU(16.0);
	vMin = interpV(0.0);
	vMax = interpV(4.0);
	posX = 8.0;
	posZ = 0.0;
	switch (dir)
	{
	case 0:
		posX = 8.0;
		posZ = 12.0;
		std::swap(uMin, uMax);
		break;
	case 1:
		posX = 0.0;
		posZ = 8.0;
		break;
	case 2:
		posX = 8.0;
		posZ = 0.0;
		break;
	case 3:
		posX = 12.0;
		posZ = 8.0;
		std::swap(uMin, uMax);
		break;
	}

	minX = static_cast<double>(x) + posX / 16.0;
	maxX = static_cast<double>(x) + (posX + 4.0) / 16.0;
	minY = static_cast<double>(y) + 0.75;
	maxY = static_cast<double>(y) + 1.0;
	minZ = static_cast<double>(z) + posZ / 16.0;
	maxZ = static_cast<double>(z) + (posZ + 4.0) / 16.0;

	if (dir == 2 || dir == 0)
	{
		tessellator.addVertexWithUV(minX, minY, minZ, uMax, vMax);
		tessellator.addVertexWithUV(minX, minY, maxZ, uMin, vMax);
		tessellator.addVertexWithUV(minX, maxY, maxZ, uMin, vMin);
		tessellator.addVertexWithUV(minX, maxY, minZ, uMax, vMin);

		tessellator.addVertexWithUV(minX, minY, maxZ, uMin, vMax);
		tessellator.addVertexWithUV(minX, minY, minZ, uMax, vMax);
		tessellator.addVertexWithUV(minX, maxY, minZ, uMax, vMin);
		tessellator.addVertexWithUV(minX, maxY, maxZ, uMin, vMin);
	}
	else if (dir == 1 || dir == 3)
	{
		tessellator.addVertexWithUV(maxX, minY, minZ, uMin, vMax);
		tessellator.addVertexWithUV(minX, minY, minZ, uMax, vMax);
		tessellator.addVertexWithUV(minX, maxY, minZ, uMax, vMin);
		tessellator.addVertexWithUV(maxX, maxY, minZ, uMin, vMin);

		tessellator.addVertexWithUV(minX, minY, minZ, uMax, vMax);
		tessellator.addVertexWithUV(maxX, minY, minZ, uMin, vMax);
		tessellator.addVertexWithUV(maxX, maxY, minZ, uMin, vMin);
		tessellator.addVertexWithUV(minX, maxY, minZ, uMax, vMin);
	}
	return true;
}

bool RenderBlocks::renderBlockQuartz(Block *block, int_t x, int_t y, int_t z)
{
	int_t meta = accessGetBlockMetadata(x, y, z);
	if (meta == 3)
	{
		eastFaceRotation = 1;
		westFaceRotation = 1;
		topFaceRotation = 1;
		bottomFaceRotation = 1;
	}
	else if (meta == 4)
	{
		southFaceRotation = 1;
		northFaceRotation = 1;
	}
	bool result = renderStandardBlock(block, x, y, z);
	southFaceRotation = 0;
	eastFaceRotation = 0;
	westFaceRotation = 0;
	northFaceRotation = 0;
	topFaceRotation = 0;
	bottomFaceRotation = 0;
	return result;
}

bool RenderBlocks::renderBlockBeacon(BlockBeacon *block, int_t x, int_t y, int_t z)
{
	float h = 0.1875f;
	overrideBlockTexture = Block::obsidian != nullptr ? Block::obsidian->getBlockTextureFromSide(0) : 37;
	block->setBlockBounds(0.125, 0.00625, 0.125, 0.875, h, 0.875);
	renderStandardBlock(block, x, y, z);
	overrideBlockTexture = Block::glass != nullptr ? Block::glass->getBlockTextureFromSide(0) : 49;
	block->setBlockBounds(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
	renderStandardBlock(block, x, y, z);
	overrideBlockTexture = block->getBeaconIcon();
	block->setBlockBounds(0.1875, h, 0.1875, 0.8125, 0.875, 0.8125);
	renderStandardBlock(block, x, y, z);
	overrideBlockTexture = -1;
	block->setBlockBounds(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
	return true;
}

bool RenderBlocks::renderBlockAnvil(BlockAnvil *block, int_t x, int_t y, int_t z)
{
	return renderBlockAnvilMetadata(block, x, y, z, accessGetBlockMetadata(x, y, z));
}

bool RenderBlocks::renderBlockAnvilMetadata(BlockAnvil *block, int_t x, int_t y, int_t z, int_t metadata)
{
	Tessellator &tessellator = Tessellator::instance;
	tessellator.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
	tessellator.setColorOpaque_F(1.0f, 1.0f, 1.0f);
	return renderBlockAnvilOrient(block, x, y, z, metadata, false);
}

bool RenderBlocks::renderBlockAnvilOrient(BlockAnvil *block, int_t x, int_t y, int_t z, int_t metadata, bool inventory)
{
	int_t dir = inventory ? 0 : (metadata & 3);
	bool rotated = false;
	float yOffset = 0.0f;
	switch (dir)
	{
	case 0:
		southFaceRotation = 2;
		northFaceRotation = 1;
		topFaceRotation = 3;
		bottomFaceRotation = 3;
		break;
	case 1:
		eastFaceRotation = 1;
		westFaceRotation = 2;
		topFaceRotation = 2;
		bottomFaceRotation = 1;
		rotated = true;
		break;
	case 2:
		southFaceRotation = 1;
		northFaceRotation = 2;
		break;
	case 3:
		eastFaceRotation = 2;
		westFaceRotation = 1;
		topFaceRotation = 1;
		bottomFaceRotation = 2;
		rotated = true;
		break;
	}

	yOffset = renderBlockAnvilRotate(block, x, y, z, 0, yOffset, 0.75f, 0.25f, 0.75f, rotated, inventory, metadata);
	yOffset = renderBlockAnvilRotate(block, x, y, z, 1, yOffset, 0.5f, 0.0625f, 0.625f, rotated, inventory, metadata);
	yOffset = renderBlockAnvilRotate(block, x, y, z, 2, yOffset, 0.25f, 0.3125f, 0.5f, rotated, inventory, metadata);
	yOffset = renderBlockAnvilRotate(block, x, y, z, 3, yOffset, 0.625f, 0.375f, 1.0f, rotated, inventory, metadata);
	block->setBlockBounds(0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
	eastFaceRotation = 0;
	westFaceRotation = 0;
	southFaceRotation = 0;
	northFaceRotation = 0;
	topFaceRotation = 0;
	bottomFaceRotation = 0;
	return true;
}

float RenderBlocks::renderBlockAnvilRotate(BlockAnvil *block, int_t x, int_t y, int_t z, int_t part, float yOffset, float sizeX, float sizeY, float sizeZ, bool rotated, bool inventory, int_t metadata)
{
	if (rotated)
	{
		float temp = sizeX;
		sizeX = sizeZ;
		sizeZ = temp;
	}
	sizeX /= 2.0f;
	sizeZ /= 2.0f;
	block->field_82521_b = part;
	block->setBlockBounds(0.5f - sizeX, yOffset, 0.5f - sizeZ, 0.5f + sizeX, yOffset + sizeY, 0.5f + sizeZ);
	if (inventory)
	{
		Tessellator &tessellator = Tessellator::instance;
		tessellator.startDrawingQuads();
		tessellator.setNormal(0.0f, -1.0f, 0.0f);
		renderBottomFace(block, 0.0, 0.0, 0.0, block->getBlockTextureFromSideAndMetadata(0, metadata));
		tessellator.setNormal(0.0f, 1.0f, 0.0f);
		renderTopFace(block, 0.0, 0.0, 0.0, block->getBlockTextureFromSideAndMetadata(1, metadata));
		tessellator.setNormal(0.0f, 0.0f, -1.0f);
		renderEastFace(block, 0.0, 0.0, 0.0, block->getBlockTextureFromSideAndMetadata(2, metadata));
		tessellator.setNormal(0.0f, 0.0f, 1.0f);
		renderWestFace(block, 0.0, 0.0, 0.0, block->getBlockTextureFromSideAndMetadata(3, metadata));
		tessellator.setNormal(-1.0f, 0.0f, 0.0f);
		renderNorthFace(block, 0.0, 0.0, 0.0, block->getBlockTextureFromSideAndMetadata(4, metadata));
		tessellator.setNormal(1.0f, 0.0f, 0.0f);
		renderSouthFace(block, 0.0, 0.0, 0.0, block->getBlockTextureFromSideAndMetadata(5, metadata));
		tessellator.draw();
	}
	else
	{
		renderStandardBlock(block, x, y, z);
	}
	return yOffset + sizeY;
}


struct TripWireVec3
{
	double xCoord;
	double yCoord;
	double zCoord;

	TripWireVec3(double x = 0.0, double y = 0.0, double z = 0.0) : xCoord(x), yCoord(y), zCoord(z) {}

	void rotateAroundX(float f)
	{
		float f1 = MathHelper::cos(f);
		float f2 = MathHelper::sin(f);
		double d1 = yCoord * (double)f1 + zCoord * (double)f2;
		double d2 = zCoord * (double)f1 - yCoord * (double)f2;
		yCoord = d1;
		zCoord = d2;
	}

	void rotateAroundY(float f)
	{
		float f1 = MathHelper::cos(f);
		float f2 = MathHelper::sin(f);
		double d0 = xCoord * (double)f1 + zCoord * (double)f2;
		double d2 = zCoord * (double)f1 - xCoord * (double)f2;
		xCoord = d0;
		zCoord = d2;
	}
};

bool RenderBlocks::renderBlockTripWireSource(Block *block, int_t x, int_t y, int_t z)
{
	Tessellator &tess = Tessellator::instance;
	int_t meta = accessGetBlockMetadata(x, y, z);
	int_t dir = meta & 3;
	bool connected = (meta & 4) == 4;
	bool powered = (meta & 8) == 8;
	bool suspended = !blockAccess->isBlockNormalCube(x, y - 1, z);
	bool hasOverride = overrideBlockTexture >= 0;
	if (!hasOverride)
	{
		overrideBlockTexture = Block::planks != nullptr ? Block::planks->getBlockTextureFromSide(0) : 4;
	}

	float var12 = 0.25f;
	float var13 = 0.125f;
	float var14 = 0.125f;
	float var15 = 0.3f - var12;
	float var16 = 0.3f + var12;
	if (dir == 2)
	{
		block->setBlockBounds(0.5f - var13, var15, 1.0f - var14, 0.5f + var13, var16, 1.0f);
	}
	else if (dir == 0)
	{
		block->setBlockBounds(0.5f - var13, var15, 0.0f, 0.5f + var13, var16, var14);
	}
	else if (dir == 1)
	{
		block->setBlockBounds(1.0f - var14, var15, 0.5f - var13, 1.0f, var16, 0.5f + var13);
	}
	else if (dir == 3)
	{
		block->setBlockBounds(0.0f, var15, 0.5f - var13, var14, var16, 0.5f + var13);
	}

	renderStandardBlock(block, x, y, z);
	if (!hasOverride)
	{
		overrideBlockTexture = -1;
	}
	block->setBlockBoundsBasedOnState(blockAccess, x, y, z);

	tess.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
	float color = 1.0f;
	tess.setColorOpaque_F(color, color, color);

	int_t tex = hasOverride ? overrideBlockTexture : block->getBlockTextureFromSide(0);
	float tileX = static_cast<float>((tex & 15) << 4);
	float tileY = static_cast<float>(tex & 240);
	auto interpU = [tileX](double t) -> tess_coord_t { return static_cast<tess_coord_t>((tileX + static_cast<float>(t)) / 256.0f); };
	auto interpV = [tileY](double t) -> tess_coord_t { return static_cast<tess_coord_t>((tileY + static_cast<float>(t)) / 256.0f); };

	TripWireVec3 pts[8];
	float rX = 0.046875f;
	float rZ = 0.046875f;
	float rY = 0.3125f;
	pts[0] = TripWireVec3(-rX, 0.0, -rZ);
	pts[1] = TripWireVec3( rX, 0.0, -rZ);
	pts[2] = TripWireVec3( rX, 0.0,  rZ);
	pts[3] = TripWireVec3(-rX, 0.0,  rZ);
	pts[4] = TripWireVec3(-rX, rY,  -rZ);
	pts[5] = TripWireVec3( rX, rY,  -rZ);
	pts[6] = TripWireVec3( rX, rY,   rZ);
	pts[7] = TripWireVec3(-rX, rY,   rZ);

	for (int_t i = 0; i < 8; ++i)
	{
		pts[i].zCoord += 0.0625;
		if (powered)
		{
			pts[i].rotateAroundX(3.14159265f / 6.0f);
			pts[i].yCoord -= 0.4375;
		}
		else if (connected)
		{
			pts[i].rotateAroundX(0.08726647f);
			pts[i].yCoord -= 0.4375;
		}
		else
		{
			pts[i].rotateAroundX(-3.14159265f * 2.0f / 9.0f);
			pts[i].yCoord -= 0.375;
		}

		pts[i].rotateAroundX(3.14159265f / 2.0f);
		if (dir == 2) pts[i].rotateAroundY(0.0f);
		if (dir == 0) pts[i].rotateAroundY(3.14159265f);
		if (dir == 1) pts[i].rotateAroundY(3.14159265f / 2.0f);
		if (dir == 3) pts[i].rotateAroundY(-3.14159265f / 2.0f);

		pts[i].xCoord += (double)x + 0.5;
		pts[i].yCoord += (double)((float)y + 0.3125f);
		pts[i].zCoord += (double)z + 0.5;
	}

	TripWireVec3 *v0 = nullptr, *v1 = nullptr, *v2 = nullptr, *v3 = nullptr;
	tess_coord_t minU = 0, minV = 0, maxU = 0, maxV = 0;

	for (int_t face = 0; face < 6; ++face)
	{
		if (face == 0)
		{
			v0 = &pts[0]; v1 = &pts[1]; v2 = &pts[2]; v3 = &pts[3];
			minU = interpU(7.0); minV = interpV(9.0); maxU = interpU(9.0); maxV = interpV(11.0);
		}
		else if (face == 1)
		{
			v0 = &pts[7]; v1 = &pts[6]; v2 = &pts[5]; v3 = &pts[4];
		}
		else if (face == 2)
		{
			v0 = &pts[1]; v1 = &pts[0]; v2 = &pts[4]; v3 = &pts[5];
			minU = interpU(7.0); minV = interpV(9.0); maxU = interpU(9.0); maxV = interpV(16.0);
		}
		else if (face == 3)
		{
			v0 = &pts[2]; v1 = &pts[1]; v2 = &pts[5]; v3 = &pts[6];
		}
		else if (face == 4)
		{
			v0 = &pts[3]; v1 = &pts[2]; v2 = &pts[6]; v3 = &pts[7];
		}
		else if (face == 5)
		{
			v0 = &pts[0]; v1 = &pts[3]; v2 = &pts[7]; v3 = &pts[4];
		}

		tess.addVertexWithUV(v0->xCoord, v0->yCoord, v0->zCoord, minU, maxV);
		tess.addVertexWithUV(v1->xCoord, v1->yCoord, v1->zCoord, maxU, maxV);
		tess.addVertexWithUV(v2->xCoord, v2->yCoord, v2->zCoord, maxU, minV);
		tess.addVertexWithUV(v3->xCoord, v3->yCoord, v3->zCoord, minU, minV);
	}

	float hookW = 0.09375f;
	float hookD = 0.09375f;
	float hookH = 0.03125f;
	pts[0] = TripWireVec3(-hookW, 0.0, -hookD);
	pts[1] = TripWireVec3( hookW, 0.0, -hookD);
	pts[2] = TripWireVec3( hookW, 0.0,  hookD);
	pts[3] = TripWireVec3(-hookW, 0.0,  hookD);
	pts[4] = TripWireVec3(-hookW, hookH, -hookD);
	pts[5] = TripWireVec3( hookW, hookH, -hookD);
	pts[6] = TripWireVec3( hookW, hookH,  hookD);
	pts[7] = TripWireVec3(-hookW, hookH,  hookD);

	for (int_t i = 0; i < 8; ++i)
	{
		pts[i].zCoord += 0.21875;
		if (powered)
		{
			pts[i].yCoord -= 0.09375;
			pts[i].zCoord -= 0.1625;
			pts[i].rotateAroundX(0.0f);
		}
		else if (connected)
		{
			pts[i].yCoord += 0.015625;
			pts[i].zCoord -= 0.171875;
			pts[i].rotateAroundX(0.17453294f);
		}
		else
		{
			pts[i].rotateAroundX(0.87266463f);
		}

		if (dir == 2) pts[i].rotateAroundY(0.0f);
		if (dir == 0) pts[i].rotateAroundY(3.14159265f);
		if (dir == 1) pts[i].rotateAroundY(3.14159265f / 2.0f);
		if (dir == 3) pts[i].rotateAroundY(-3.14159265f / 2.0f);

		pts[i].xCoord += (double)x + 0.5;
		pts[i].yCoord += (double)((float)y + 0.3125f);
		pts[i].zCoord += (double)z + 0.5;
	}

	for (int_t face = 0; face < 6; ++face)
	{
		if (face == 0)
		{
			v0 = &pts[0]; v1 = &pts[1]; v2 = &pts[2]; v3 = &pts[3];
			minU = interpU(5.0); minV = interpV(3.0); maxU = interpU(11.0); maxV = interpV(9.0);
		}
		else if (face == 1)
		{
			v0 = &pts[7]; v1 = &pts[6]; v2 = &pts[5]; v3 = &pts[4];
		}
		else if (face == 2)
		{
			v0 = &pts[1]; v1 = &pts[0]; v2 = &pts[4]; v3 = &pts[5];
			minU = interpU(5.0); minV = interpV(3.0); maxU = interpU(11.0); maxV = interpV(5.0);
		}
		else if (face == 3)
		{
			v0 = &pts[2]; v1 = &pts[1]; v2 = &pts[5]; v3 = &pts[6];
		}
		else if (face == 4)
		{
			v0 = &pts[3]; v1 = &pts[2]; v2 = &pts[6]; v3 = &pts[7];
		}
		else if (face == 5)
		{
			v0 = &pts[0]; v1 = &pts[3]; v2 = &pts[7]; v3 = &pts[4];
		}

		tess.addVertexWithUV(v0->xCoord, v0->yCoord, v0->zCoord, minU, maxV);
		tess.addVertexWithUV(v1->xCoord, v1->yCoord, v1->zCoord, maxU, maxV);
		tess.addVertexWithUV(v2->xCoord, v2->yCoord, v2->zCoord, maxU, minV);
		tess.addVertexWithUV(v3->xCoord, v3->yCoord, v3->zCoord, minU, minV);
	}

	if (connected)
	{
		double hookY = pts[0].yCoord;
		float stringW = 0.03125f;
		float stringMin = 0.5f - stringW / 2.0f;
		float stringMax = stringMin + stringW;
		int_t wireTex = Block::tripWire != nullptr ? Block::tripWire->getBlockTextureFromSide(0) : TEX152_BLOCK_ICON("tripWire.png");
		float wTileX = static_cast<float>((wireTex & 15) << 4);
		float wTileY = static_cast<float>(wireTex & 240);
		auto wInterpU = [wTileX](double t) -> tess_coord_t { return static_cast<tess_coord_t>((wTileX + static_cast<float>(t)) / 256.0f); };
		auto wInterpV = [wTileY](double t) -> tess_coord_t { return static_cast<tess_coord_t>((wTileY + static_cast<float>(t)) / 256.0f); };

		tess_coord_t sMinU = wInterpU(0.0);
		tess_coord_t sMinV = wInterpV(connected ? 2.0 : 0.0);
		tess_coord_t sMaxU = wInterpU(16.0);
		tess_coord_t sMaxV = wInterpV(connected ? 4.0 : 2.0);
		double stringHeight = (double)(suspended ? 3.5f : 1.5f) / 16.0;
		color = block->getBlockBrightness(blockAccess, x, y, z) * 0.75f;
		tess.setColorOpaque_F(color, color, color);

		if (dir == 2)
		{
			tess.addVertexWithUV((double)((float)x + stringMin), (double)y + stringHeight, (double)z + 0.25, sMinU, sMinV);
			tess.addVertexWithUV((double)((float)x + stringMax), (double)y + stringHeight, (double)z + 0.25, sMinU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMax), (double)y + stringHeight, (double)z, sMaxU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMin), (double)y + stringHeight, (double)z, sMaxU, sMinV);

			tess.addVertexWithUV((double)((float)x + stringMin), hookY, (double)z + 0.5, sMinU, sMinV);
			tess.addVertexWithUV((double)((float)x + stringMax), hookY, (double)z + 0.5, sMinU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMax), (double)y + stringHeight, (double)z + 0.25, sMaxU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMin), (double)y + stringHeight, (double)z + 0.25, sMaxU, sMinV);
		}
		else if (dir == 0)
		{
			tess.addVertexWithUV((double)((float)x + stringMin), (double)y + stringHeight, (double)z + 0.75, sMinU, sMinV);
			tess.addVertexWithUV((double)((float)x + stringMax), (double)y + stringHeight, (double)z + 0.75, sMinU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMax), hookY, (double)z + 0.5, sMaxU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMin), hookY, (double)z + 0.5, sMaxU, sMinV);

			tess.addVertexWithUV((double)((float)x + stringMin), (double)y + stringHeight, (double)(z + 1), sMinU, sMinV);
			tess.addVertexWithUV((double)((float)x + stringMax), (double)y + stringHeight, (double)(z + 1), sMinU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMax), (double)y + stringHeight, (double)z + 0.75, sMaxU, sMaxV);
			tess.addVertexWithUV((double)((float)x + stringMin), (double)y + stringHeight, (double)z + 0.75, sMaxU, sMinV);
		}
		else if (dir == 1)
		{
			tess.addVertexWithUV((double)x, (double)y + stringHeight, (double)((float)z + stringMax), sMinU, sMaxV);
			tess.addVertexWithUV((double)x + 0.25, (double)y + stringHeight, (double)((float)z + stringMax), sMaxU, sMaxV);
			tess.addVertexWithUV((double)x + 0.25, (double)y + stringHeight, (double)((float)z + stringMin), sMaxU, sMinV);
			tess.addVertexWithUV((double)x, (double)y + stringHeight, (double)((float)z + stringMin), sMinU, sMinV);

			tess.addVertexWithUV((double)x + 0.25, (double)y + stringHeight, (double)((float)z + stringMax), sMinU, sMaxV);
			tess.addVertexWithUV((double)x + 0.5, hookY, (double)((float)z + stringMax), sMaxU, sMaxV);
			tess.addVertexWithUV((double)x + 0.5, hookY, (double)((float)z + stringMin), sMaxU, sMinV);
			tess.addVertexWithUV((double)x + 0.25, (double)y + stringHeight, (double)((float)z + stringMin), sMinU, sMinV);
		}
		else
		{
			tess.addVertexWithUV((double)x + 0.5, hookY, (double)((float)z + stringMax), sMinU, sMaxV);
			tess.addVertexWithUV((double)x + 0.75, (double)y + stringHeight, (double)((float)z + stringMax), sMaxU, sMaxV);
			tess.addVertexWithUV((double)x + 0.75, (double)y + stringHeight, (double)((float)z + stringMin), sMaxU, sMinV);
			tess.addVertexWithUV((double)x + 0.5, hookY, (double)((float)z + stringMin), sMinU, sMinV);

			tess.addVertexWithUV((double)x + 0.75, (double)y + stringHeight, (double)((float)z + stringMax), sMinU, sMaxV);
			tess.addVertexWithUV((double)(x + 1), (double)y + stringHeight, (double)((float)z + stringMax), sMaxU, sMaxV);
			tess.addVertexWithUV((double)(x + 1), (double)y + stringHeight, (double)((float)z + stringMin), sMaxU, sMinV);
			tess.addVertexWithUV((double)x + 0.75, (double)y + stringHeight, (double)((float)z + stringMin), sMaxU, sMinV);
		}
	}
	return true;
}

bool RenderBlocks::renderBlockTripWire(Block *block, int_t x, int_t y, int_t z)
{
	Tessellator &tess = Tessellator::instance;
	int_t meta = accessGetBlockMetadata(x, y, z);
	bool connected = (meta & 4) == 4;
	bool suspended = (meta & 2) == 2;
	int_t tex = overrideBlockTexture >= 0 ? overrideBlockTexture : block->getBlockTextureFromSide(0);

	tess.setBrightness(block->getMixedBrightnessForBlock(blockAccess, x, y, z));
	float color = block->getBlockBrightness(blockAccess, x, y, z) * 0.75f;
	tess.setColorOpaque_F(color, color, color);

	float tileX = static_cast<float>((tex & 15) << 4);
	float tileY = static_cast<float>(tex & 240);
	auto interpU = [tileX](double t) -> tess_coord_t { return static_cast<tess_coord_t>((tileX + static_cast<float>(t)) / 256.0f); };
	auto interpV = [tileY](double t) -> tess_coord_t { return static_cast<tess_coord_t>((tileY + static_cast<float>(t)) / 256.0f); };

	tess_coord_t minU = interpU(0.0);
	tess_coord_t minV = interpV(connected ? 2.0 : 0.0);
	tess_coord_t maxU = interpU(16.0);
	tess_coord_t maxV = interpV(connected ? 4.0 : 2.0);
	double height = (double)(suspended ? 3.5f : 1.5f) / 16.0;

	bool connE = BlockTripWire::isConnected(blockAccess, x, y, z, meta, 1);
	bool connW = BlockTripWire::isConnected(blockAccess, x, y, z, meta, 3);
	bool connS = BlockTripWire::isConnected(blockAccess, x, y, z, meta, 2);
	bool connN = BlockTripWire::isConnected(blockAccess, x, y, z, meta, 0);

	float stringW = 0.03125f;
	float stringMin = 0.5f - stringW / 2.0f;
	float stringMax = stringMin + stringW;

	if (!connS && !connW && !connN && !connE)
	{
		connS = true;
		connN = true;
	}

	if (connS)
	{
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.25, minU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.25, minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.25, minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.25, minU, minV);
	}

	if (connS || (connN && !connW && !connE))
	{
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.5, minU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.5, minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.25, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.25, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.25, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.25, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.5, minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.5, minU, minV);
	}

	if (connN || (connS && !connW && !connE))
	{
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.75, minU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.75, minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.5, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.5, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.5, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.5, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.75, minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.75, minU, minV);
	}

	if (connN)
	{
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)(z + 1), minU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)(z + 1), minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.75, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.75, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)z + 0.75, maxU, minV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)z + 0.75, maxU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMax), (double)y + height, (double)(z + 1), minU, maxV);
		tess.addVertexWithUV((double)((float)x + stringMin), (double)y + height, (double)(z + 1), minU, minV);
	}

	if (connE)
	{
		tess.addVertexWithUV((double)x, (double)y + height, (double)((float)z + stringMax), minU, maxV);
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)x, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)x, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)x, (double)y + height, (double)((float)z + stringMax), minU, maxV);
	}

	if (connE || (connW && !connS && !connN))
	{
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMax), minU, maxV);
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)x + 0.25, (double)y + height, (double)((float)z + stringMax), minU, maxV);
	}

	if (connW || (connE && !connS && !connN))
	{
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMax), minU, maxV);
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)x + 0.5, (double)y + height, (double)((float)z + stringMax), minU, maxV);
	}

	if (connW)
	{
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMax), minU, maxV);
		tess.addVertexWithUV((double)(x + 1), (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)(x + 1), (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMin), minU, minV);
		tess.addVertexWithUV((double)(x + 1), (double)y + height, (double)((float)z + stringMin), maxU, minV);
		tess.addVertexWithUV((double)(x + 1), (double)y + height, (double)((float)z + stringMax), maxU, maxV);
		tess.addVertexWithUV((double)x + 0.75, (double)y + height, (double)((float)z + stringMax), minU, maxV);
	}

	return true;
}

