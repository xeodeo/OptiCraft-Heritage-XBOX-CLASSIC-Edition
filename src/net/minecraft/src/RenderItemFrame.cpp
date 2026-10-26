#include "RenderItemFrame.h"
#include "EntityItemFrame.h"
#include "RenderManager.h"
#include "ItemRenderer.h"
#include "ItemStack.h"
#include "platform/RenderAPI.h"
void RenderItemFrame::doRender(Entity*e,double x,double y,double z,float yaw,float partial){EntityItemFrame*f=dynamic_cast<EntityItemFrame*>(e);if(f)renderItemFrame(f,x,y,z,yaw,partial);}
void RenderItemFrame::renderItemFrame(EntityItemFrame*f,double x,double y,double z,float yaw,float partial){renderPushMatrix();renderTranslate((float)x,(float)y,(float)z);renderRotate(yaw,0,1,0);bindFrameTexture();drawFrameBack();drawFrameFront();drawFrameEdges();renderDisplayedItem(f);restoreRenderState();}
void RenderItemFrame::renderDisplayedItem(EntityItemFrame*f){ItemStack*s=f->getDisplayedItem();if(!s||!RenderManager::instance||!RenderManager::instance->itemRenderer)return;renderPushMatrix();orientItem(f->getRotation());renderScale(.5f,.5f,.5f);RenderManager::instance->itemRenderer->renderItem(RenderManager::instance->livingPlayer,s);renderPopMatrix();}
void RenderItemFrame::orientItem(int r){renderTranslate(0,0,-.04f);renderRotate((float)(r*90),0,0,1);}
void RenderItemFrame::bindFrameTexture(){loadTexture("/item/item_frame.png");}
void RenderItemFrame::drawFrameBack(){renderEnable(RenderCapability::RescaleNormal);}
void RenderItemFrame::drawFrameFront(){renderDisable(RenderCapability::RescaleNormal);}
void RenderItemFrame::drawFrameEdges(){renderEnable(RenderCapability::RescaleNormal);}
void RenderItemFrame::restoreRenderState(){renderDisable(RenderCapability::RescaleNormal);renderPopMatrix();}
void RenderItemFrame::drawItemShadow(float scale){if(scale<=0)return;renderDisable(RenderCapability::Lighting);renderEnable(RenderCapability::Blend);renderScale(scale,scale,scale);renderDisable(RenderCapability::Blend);renderEnable(RenderCapability::Lighting);}
void RenderItemFrame::drawItemHighlight(int rotation){if((rotation&1)!=0)renderRotate(90,0,0,1);}
void RenderItemFrame::setItemBrightness(float brightness){if(brightness<0)brightness=0;if(brightness>1)brightness=1;renderColor4f(brightness,brightness,brightness,1);}
float RenderItemFrame::itemScale(const ItemStack*stack)const{if(stack==nullptr)return 0.0f;return const_cast<ItemStack*>(stack)->isItemStackDamageable()?0.5f:0.625f;}
bool RenderItemFrame::hasDisplayedItem(const EntityItemFrame*frame)const{return frame!=nullptr&&frame->getDisplayedItem()!=nullptr;}
void RenderItemFrame::translateToFrameSurface(const EntityItemFrame*frame){if(frame==nullptr)return;renderTranslate(0,0,-0.0625f);}
void RenderItemFrame::rotateForFacing(int direction){if(direction<0)direction=0;direction&=3;renderRotate((float)(direction*90),0,1,0);}
void RenderItemFrame::finishItemTransform(){renderScale(1,1,1);}
void RenderItemFrame::renderItemFace(float size)
{
    if (size <= 0.0f) return;
    renderScale(size, size, size);
}
void RenderItemFrame::renderItemBack(float size)
{
    renderPushMatrix();
    renderTranslate(0.0f, 0.0f, 0.01f);
    renderItemFace(size);
    renderPopMatrix();
}
void RenderItemFrame::renderItemSides(float size)
{
    renderPushMatrix();
    renderScale(size, size, size);
    renderRotate(90.0f, 1.0f, 0.0f, 0.0f);
    renderPopMatrix();
}
void RenderItemFrame::renderRotationMarker(int rotation)
{
    int normalized = rotation & 3;
    if (normalized == 0) return;
    renderRotate((float)(normalized * 90), 0.0f, 0.0f, 1.0f);
}
void RenderItemFrame::pushItemLighting()
{
    renderEnable(RenderCapability::Lighting);
    renderEnable(RenderCapability::RescaleNormal);
}
void RenderItemFrame::popItemLighting()
{
    renderDisable(RenderCapability::RescaleNormal);
    renderDisable(RenderCapability::Lighting);
}
void RenderItemFrame::applyDepthOffset(float offset)
{
    renderTranslate(0.0f, 0.0f, offset);
}
void RenderItemFrame::resetDepthOffset()
{
    renderTranslate(0.0f, 0.0f, 0.0f);
}
