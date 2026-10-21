#include "TileEntitySkullRenderer.h"

#include "TileEntitySkull.h"
#include "platform/RenderAPI.h"

TileEntitySkullRenderer *TileEntitySkullRenderer::skullRenderer = nullptr;

TileEntitySkullRenderer::TileEntitySkullRenderer()
	: field_82396_c(0, 0, 64, 32)
	, field_82395_d(0, 0, 64, 64)
{
	skullRenderer = this;
}

TileEntitySkullRenderer::~TileEntitySkullRenderer()
{
	if (skullRenderer == this)
	{
		skullRenderer = nullptr;
	}
}

void TileEntitySkullRenderer::renderTileEntityAt(TileEntity *tileentity, double d, double d1, double d2, float f)
{
	(void)f;
	TileEntitySkull *skull = dynamic_cast<TileEntitySkull *>(tileentity);
	if (skull != nullptr)
	{
		const int_t side = skull->getBlockMetadata() & 7;
		const float rot = static_cast<float>(skull->getSkullRotation() * 360) / 16.0f;
		renderSkull(static_cast<float>(d), static_cast<float>(d1), static_cast<float>(d2),
		            side, rot, skull->getSkullType(), skull->getExtraType());
	}
}

void TileEntitySkullRenderer::renderSkull(float x, float y, float z, int_t side, float rotation, int_t type, const std::string &extra)
{
	(void)extra;
	ModelSkeletonHead *model = &field_82396_c;
	switch (type)
	{
	case 0:
	default:
		bindTextureByName("/mob/skeleton.png");
		break;
	case 1:
		bindTextureByName("/mob/skeleton_wither.png");
		break;
	case 2:
		bindTextureByName("/mob/zombie.png");
		model = &field_82395_d;
		break;
	case 3:
		bindTextureByName("/mob/char.png");
		break;
	case 4:
		bindTextureByName("/mob/creeper.png");
		break;
	}

	renderPushMatrix();
	renderDisable(RenderCapability::CullFace);
	if (side != 1)
	{
		switch (side)
		{
		case 2:
			renderTranslate(x + 0.5f, y + 0.25f, z + 0.74f);
			break;
		case 3:
			renderTranslate(x + 0.5f, y + 0.25f, z + 0.26f);
			rotation = 180.0f;
			break;
		case 4:
			renderTranslate(x + 0.74f, y + 0.25f, z + 0.5f);
			rotation = 270.0f;
			break;
		case 5:
		default:
			renderTranslate(x + 0.26f, y + 0.25f, z + 0.5f);
			rotation = 90.0f;
			break;
		}
	}
	else
	{
		renderTranslate(x + 0.5f, y, z + 0.5f);
	}

	const float scale = 0.0625f;
	renderScale(-1.0f, -1.0f, 1.0f);
	model->render(nullptr, 0.0f, 0.0f, 0.0f, rotation, 0.0f, scale);
	renderEnable(RenderCapability::CullFace);
	renderPopMatrix();
}
