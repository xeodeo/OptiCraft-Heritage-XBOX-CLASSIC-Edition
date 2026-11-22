#ifndef RENDERPLAYER_H
#define RENDERPLAYER_H

#include "RenderLiving.h"

class EntityPlayer;
class ModelBiped;
class ModelPlayer;

class RenderPlayer : public RenderLiving {
public:
    RenderPlayer();
    ~RenderPlayer() override;

    void renderPlayer(EntityPlayer* entityPlayer, double d, double d1, double d2, float f, float f1);

    virtual void doRenderLiving(EntityLiving* entityLiving, double d, double d1, double d2, float f, float f1) override;

    virtual void doRender(Entity* entity, double d, double d1, double d2, float f, float f1) override;
	// player: whose skin is bound (picks the 64x64/slim arm); null = classic arm.
	void drawFirstPersonHand(EntityPlayer* player = nullptr);

protected:
    void renderName(EntityPlayer* entityPlayer, double d, double d1, double d2);

    void passSpecialRender(EntityLiving* entityLiving, double d, double d1, double d2) override;

    int setArmorModel(EntityPlayer* entityPlayer, int i, float f);

    int shouldRenderPass(EntityLiving* entityLiving, int i, float f) override;

    virtual void renderSpecials(EntityPlayer* entityPlayer, float f);

    virtual void renderEquippedItems(EntityLiving* entityLiving, float f) override;

    virtual void preRenderCallback(EntityLiving* entityLiving, float f) override;

    void translateEntity(EntityLiving* entityLiving, double d, double d1, double d2) override;
    void rotateCorpse(EntityLiving* entityLiving, float f, float f1, float f2) override;

private:
    // Model for the skin this player is drawn with: the classic 64x32
    // ModelBiped, or the 1.8 ModelPlayer (wide or slim arms) for 64x64 skins.
    ModelBiped* selectSkinModel(EntityPlayer* entityPlayer);

    ModelBiped* modelBipedMain;
    ModelBiped* modelClassic;
    ModelPlayer* modelPlayerWide;
    ModelPlayer* modelPlayerSlim;
    std::string skinPathScratch;
    ModelBiped* modelArmorChestplate;
    ModelBiped* modelArmor;
    int field_208_f;
};

#endif
