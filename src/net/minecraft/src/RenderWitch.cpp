#include "RenderWitch.h"

#include "Block.h"
#include "EntityLiving.h"
#include "Item.h"
#include "ItemRenderer.h"
#include "ItemStack.h"
#include "ModelRenderer.h"
#include "ModelWitch.h"
#include "RenderBlocks.h"
#include "RenderManager.h"
#include "platform/RenderAPI.h"

RenderWitch::RenderWitch()
	: RenderLiving(new ModelWitch(0.0f), 0.5f),
	  modelWitch(static_cast<ModelWitch *>(mainModel))
{
}

void RenderWitch::doRenderLiving(EntityLiving *entityLiving, double d, double d1, double d2, float f, float f1)
{
	ItemStack *heldItem = entityLiving->getHeldItem();
	modelWitch->field_82900_g = (heldItem != nullptr);
	RenderLiving::doRenderLiving(entityLiving, d, d1, d2, f, f1);
}

void RenderWitch::preRenderCallback(EntityLiving *, float)
{
	constexpr float scale = 0.9375f;
	renderScale(scale, scale, scale);
}

void RenderWitch::renderEquippedItems(EntityLiving *entityLiving, float f)
{
	renderColor3f(1.0f, 1.0f, 1.0f);
	RenderLiving::renderEquippedItems(entityLiving, f);

	ItemStack *itemStack = entityLiving->getHeldItem();
	if (itemStack != nullptr && itemStack->isValid())
	{
		Item *item = itemStack->getItem();
		if (item == nullptr)
			return;

		renderPushMatrix();
		if (mainModel != nullptr && mainModel->isChild)
		{
			constexpr float childScale = 0.5f;
			renderTranslate(0.0f, 0.625f, 0.0f);
			renderRotate(-20.0f, -1.0f, 0.0f, 0.0f);
			renderScale(childScale, childScale, childScale);
		}

		if (modelWitch->field_82898_f != nullptr)
			modelWitch->field_82898_f->postRender(0.0625f);

		renderTranslate(-0.0625f, 0.53125f, 0.21875f);

		if (itemStack->itemID >= 0 && itemStack->itemID < 256 &&
		    Block::blocksList[itemStack->itemID] != nullptr &&
		    RenderBlocks::renderItemIn3d(Block::blocksList[itemStack->itemID]->getRenderType()))
		{
			float bScale = 0.5f;
			renderTranslate(0.0f, 0.1875f, -0.3125f);
			bScale *= 0.75f;
			renderRotate(20.0f, 1.0f, 0.0f, 0.0f);
			renderRotate(45.0f, 0.0f, 1.0f, 0.0f);
			renderScale(bScale, -bScale, bScale);
		}
		else if (Item::bow != nullptr && itemStack->itemID == Item::bow->shiftedIndex)
		{
			constexpr float bowScale = 0.625f;
			renderTranslate(0.0f, 0.125f, 0.3125f);
			renderRotate(-20.0f, 0.0f, 1.0f, 0.0f);
			renderScale(bowScale, -bowScale, bowScale);
			renderRotate(-100.0f, 1.0f, 0.0f, 0.0f);
			renderRotate(45.0f, 0.0f, 1.0f, 0.0f);
		}
		else if (item->isFull3D())
		{
			constexpr float f3dScale = 0.625f;
			renderTranslate(0.0f, 0.1875f, 0.0f);
			renderScale(f3dScale, -f3dScale, f3dScale);
			renderRotate(-100.0f, 1.0f, 0.0f, 0.0f);
			renderRotate(45.0f, 0.0f, 1.0f, 0.0f);
		}
		else
		{
			constexpr float itemScale = 0.375f;
			renderTranslate(0.25f, 0.1875f, -0.1875f);
			renderScale(itemScale, itemScale, itemScale);
			renderRotate(60.0f, 0.0f, 0.0f, 1.0f);
			renderRotate(-90.0f, 1.0f, 0.0f, 0.0f);
			renderRotate(20.0f, 0.0f, 0.0f, 1.0f);
		}

		renderRotate(-15.0f, 1.0f, 0.0f, 0.0f);
		renderRotate(40.0f, 0.0f, 0.0f, 1.0f);

		if (renderManager != nullptr && renderManager->itemRenderer != nullptr)
		{
			renderManager->itemRenderer->renderItem(entityLiving, itemStack, 0);
			if (item->func_46058_c())
				renderManager->itemRenderer->renderItem(entityLiving, itemStack, 1);
		}

		renderPopMatrix();
	}
}
