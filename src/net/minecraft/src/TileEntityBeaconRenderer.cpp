#include "TileEntityBeaconRenderer.h"

#include "TileEntityBeacon.h"
#include "Tessellator.h"
#include "World.h"
#include "MathHelper.h"
#include "platform/RenderAPI.h"
#include <cmath>

TileEntityBeaconRenderer::TileEntityBeaconRenderer()
{
}

TileEntityBeaconRenderer::~TileEntityBeaconRenderer()
{
}

void TileEntityBeaconRenderer::renderTileEntityAt(TileEntity *tileentity, double x, double y, double z, float partialTick)
{
	TileEntityBeacon *beacon = dynamic_cast<TileEntityBeacon *>(tileentity);
	if (beacon != nullptr)
	{
		renderTileEntityBeaconAt(beacon, x, y, z, partialTick);
	}
}

void TileEntityBeaconRenderer::renderTileEntityBeaconAt(TileEntityBeacon *beacon, double x, double y, double z, float partialTick)
{
	float factor = beacon->func_82125_v_();
	if (factor > 0.0f)
	{
		Tessellator &tessellator = Tessellator::instance;
		bindTextureByName("/misc/beam.png");
		renderDisable(RenderCapability::Lighting);
		renderDisable(RenderCapability::CullFace);
		renderDisable(RenderCapability::Blend);
		renderDepthMask(true);
		renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::One);

		float time = 0.0f;
		if (beacon->worldObj != nullptr)
		{
			time = static_cast<float>(beacon->worldObj->getWorldTime()) + partialTick;
		}
		float vOffset = -time * 0.2f - static_cast<float>(MathHelper::floor_float(-time * 0.1f));
		char c = 1;
		double dAngle = static_cast<double>(time) * 0.025 * (1.0 - static_cast<double>(c & 1) * 2.5);
		tessellator.startDrawingQuads();
		tessellator.setColorRGBA(255, 255, 255, 32);
		double dWidth = static_cast<double>(c) * 0.2;
		double dX1 = 0.5 + std::cos(dAngle + 2.356194490192345) * dWidth;
		double dZ1 = 0.5 + std::sin(dAngle + 2.356194490192345) * dWidth;
		double dX2 = 0.5 + std::cos(dAngle + (3.141592653589793 / 4.0)) * dWidth;
		double dZ2 = 0.5 + std::sin(dAngle + (3.141592653589793 / 4.0)) * dWidth;
		double dX3 = 0.5 + std::cos(dAngle + 3.9269908169872414) * dWidth;
		double dZ3 = 0.5 + std::sin(dAngle + 3.9269908169872414) * dWidth;
		double dX4 = 0.5 + std::cos(dAngle + 5.497787143782138) * dWidth;
		double dZ4 = 0.5 + std::sin(dAngle + 5.497787143782138) * dWidth;
		double dHeight = static_cast<double>(256.0f * factor);
		double uMin = 0.0;
		double uMax = 1.0;
		double vMin = static_cast<double>(-1.0f + vOffset);
		double vMax = static_cast<double>(256.0f * factor) * (0.5 / dWidth) + vMin;

		tessellator.addVertexWithUV(x + dX1, y + dHeight, z + dZ1, uMax, vMax);
		tessellator.addVertexWithUV(x + dX1, y, z + dZ1, uMax, vMin);
		tessellator.addVertexWithUV(x + dX2, y, z + dZ2, uMin, vMin);
		tessellator.addVertexWithUV(x + dX2, y + dHeight, z + dZ2, uMin, vMax);

		tessellator.addVertexWithUV(x + dX4, y + dHeight, z + dZ4, uMax, vMax);
		tessellator.addVertexWithUV(x + dX4, y, z + dZ4, uMax, vMin);
		tessellator.addVertexWithUV(x + dX3, y, z + dZ3, uMin, vMin);
		tessellator.addVertexWithUV(x + dX3, y + dHeight, z + dZ3, uMin, vMax);

		tessellator.addVertexWithUV(x + dX2, y + dHeight, z + dZ2, uMax, vMax);
		tessellator.addVertexWithUV(x + dX2, y, z + dZ2, uMax, vMin);
		tessellator.addVertexWithUV(x + dX4, y, z + dZ4, uMin, vMin);
		tessellator.addVertexWithUV(x + dX4, y + dHeight, z + dZ4, uMin, vMax);

		tessellator.addVertexWithUV(x + dX3, y + dHeight, z + dZ3, uMax, vMax);
		tessellator.addVertexWithUV(x + dX3, y, z + dZ3, uMin, vMin);
		tessellator.addVertexWithUV(x + dX1, y, z + dZ1, uMin, vMin);
		tessellator.addVertexWithUV(x + dX1, y + dHeight, z + dZ1, uMin, vMax);
		tessellator.draw();

		renderEnable(RenderCapability::Blend);
		renderBlendFunc(RenderBlendFactor::SrcAlpha, RenderBlendFactor::OneMinusSrcAlpha);
		renderDepthMask(false);
		tessellator.startDrawingQuads();
		tessellator.setColorRGBA(255, 255, 255, 32);
		double bX1 = 0.2;
		double bZ1 = 0.2;
		double bX2 = 0.8;
		double bZ2 = 0.2;
		double bX3 = 0.2;
		double bZ3 = 0.8;
		double bX4 = 0.8;
		double bZ4 = 0.8;
		double bHeight = static_cast<double>(256.0f * factor);
		double bVMin = static_cast<double>(-1.0f + vOffset);
		double bVMax = static_cast<double>(256.0f * factor) + bVMin;

		tessellator.addVertexWithUV(x + bX1, y + bHeight, z + bZ1, uMax, bVMax);
		tessellator.addVertexWithUV(x + bX1, y, z + bZ1, uMax, bVMin);
		tessellator.addVertexWithUV(x + bX2, y, z + bZ2, uMin, bVMin);
		tessellator.addVertexWithUV(x + bX2, y + bHeight, z + bZ2, uMin, bVMax);

		tessellator.addVertexWithUV(x + bX4, y + bHeight, z + bZ4, uMax, bVMax);
		tessellator.addVertexWithUV(x + bX4, y, z + bZ4, uMax, bVMin);
		tessellator.addVertexWithUV(x + bX3, y, z + bZ3, uMin, bVMin);
		tessellator.addVertexWithUV(x + bX3, y + bHeight, z + bZ3, uMin, bVMax);

		tessellator.addVertexWithUV(x + bX2, y + bHeight, z + bZ2, uMax, bVMax);
		tessellator.addVertexWithUV(x + bX2, y, z + bZ2, uMin, bVMin);
		tessellator.addVertexWithUV(x + bX4, y, z + bZ4, uMin, bVMin);
		tessellator.addVertexWithUV(x + bX4, y + bHeight, z + bZ4, uMin, bVMax);

		tessellator.addVertexWithUV(x + bX3, y + bHeight, z + bZ3, uMax, bVMax);
		tessellator.addVertexWithUV(x + bX3, y, z + bZ3, uMin, bVMin);
		tessellator.addVertexWithUV(x + bX1, y, z + bZ1, uMin, bVMin);
		tessellator.addVertexWithUV(x + bX1, y + bHeight, z + bZ1, uMin, bVMax);
		tessellator.draw();

		renderEnable(RenderCapability::Lighting);
		renderEnable(RenderCapability::Texture2D);
		renderDepthMask(true);
	}
}
