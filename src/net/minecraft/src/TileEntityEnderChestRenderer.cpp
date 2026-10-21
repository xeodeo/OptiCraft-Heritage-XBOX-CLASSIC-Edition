#include "TileEntityEnderChestRenderer.h"

#include "ModelChest.h"
#include "ModelRenderer.h"
#include "TileEntityEnderChest.h"
#include "platform/RenderAPI.h"

TileEntityEnderChestRenderer::TileEntityEnderChestRenderer()
	: theEnderChestModel(new ModelChest())
{
}

TileEntityEnderChestRenderer::~TileEntityEnderChestRenderer()
{
	delete theEnderChestModel;
}

void TileEntityEnderChestRenderer::renderTileEntityAt(TileEntity *tileentity, double d, double d1, double d2, float f)
{
	TileEntityEnderChest *chest = dynamic_cast<TileEntityEnderChest *>(tileentity);
	int_t metadata = 0;
	if (chest != nullptr && chest->worldObj != nullptr)
	{
		metadata = chest->getBlockMetadata();
	}

	bindTextureByName("/item/enderchest.png");
	renderPushMatrix();
	renderEnable(RenderCapability::RescaleNormal);
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	renderTranslate(static_cast<float>(d), static_cast<float>(d1) + 1.0f, static_cast<float>(d2) + 1.0f);
	renderScale(1.0f, -1.0f, -1.0f);
	renderTranslate(0.5f, 0.5f, 0.5f);

	float rotation = 0.0f;
	if (metadata == 2) rotation = 180.0f;
	if (metadata == 3) rotation = 0.0f;
	if (metadata == 4) rotation = 90.0f;
	if (metadata == 5) rotation = -90.0f;

	renderRotate(rotation, 0.0f, 1.0f, 0.0f);
	renderTranslate(-0.5f, -0.5f, -0.5f);

	float lid = 0.0f;
	if (chest != nullptr)
	{
		lid = chest->prevLidAngle + (chest->lidAngle - chest->prevLidAngle) * f;
	}
	lid = 1.0f - lid;
	lid = 1.0f - lid * lid * lid;
	theEnderChestModel->chestLid->rotateAngleX = -(lid * 3.14159265358979323846f / 2.0f);
	theEnderChestModel->renderAll();

	renderDisable(RenderCapability::RescaleNormal);
	renderPopMatrix();
	renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
