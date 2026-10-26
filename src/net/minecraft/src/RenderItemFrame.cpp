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
