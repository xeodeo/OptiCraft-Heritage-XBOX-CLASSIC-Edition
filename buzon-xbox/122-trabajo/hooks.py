import sys, re, os
root = sys.argv[1]  # src dir
INC = '#include "mods/adminmode/AdminHooks.h"'

def edit(rel, pairs, include=True):
    p = os.path.join(root, rel)
    s = open(p, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    for old, new in pairs:
        o = old.replace('\n', nl); n = new.replace('\n', nl)
        c = s.count(o)
        if c != 1:
            print('FAIL', rel, c, repr(old[:70])); sys.exit(1)
        s = s.replace(o, n, 1)
    if include and INC not in s:
        m = re.search(r'^#include "[^"]+"[^\n]*\n', s, re.M)
        s = s[:m.end()] + INC + nl + s[m.end():]
    open(p, 'w', encoding='utf-8', newline='').write(s)
    print('ok', rel)

N = 'net/minecraft/src/'
edit(N+'EntityPlayerSP.cpp', [
 ('\tif (movementInput != nullptr)\n\t\tmovementInput->updatePlayerMoveState(this);\n',
  '\tif (movementInput != nullptr)\n\t\tmovementInput->updatePlayerMoveState(this);\n'
  '\tif (movementInput != nullptr && AdminHooks::onFor(AdminHooks::AutoWalk, this) &&\n'
  '\t\t(mc == nullptr || mc->currentScreen == nullptr))\n\t\tmovementInput->moveForward = 1.0f;\n'),
 ('\tif (isUsingItem() && movementInput != nullptr)\n\t{\n\t\tmovementInput->moveStrafe *= 0.2f;',
  '\tif (isUsingItem() && movementInput != nullptr && !AdminHooks::onFor(AdminHooks::NoSlowDown, this))\n\t{\n\t\tmovementInput->moveStrafe *= 0.2f;'),
 ('bool EntityPlayerSP::isSneaking()\n{\n\treturn movementInput != nullptr && movementInput->sneak && !sleeping;',
  'bool EntityPlayerSP::isSneaking()\n{\n\tif (AdminHooks::onFor(AdminHooks::Sneak, this) && !sleeping)\n\t\treturn true;\n\treturn movementInput != nullptr && movementInput->sneak && !sleeping;'),
 ('bool EntityPlayerSP::pushOutOfBlocks(double d, double d1, double d2)\n{\n',
  'bool EntityPlayerSP::pushOutOfBlocks(double d, double d1, double d2)\n{\n\tif (noClip) // admin mode Freecam flies through blocks\n\t\treturn false;\n'),
])
edit(N+'Entity.cpp', [
 ('bool Entity::isEntityInsideOpaqueBlock()\n{\n',
  'bool Entity::isEntityInsideOpaqueBlock()\n{\n\tif (noClip && AdminHooks::onFor(AdminHooks::Freecam, this))\n\t\treturn false;\n'),
 ('\tif (isInWeb)\n\t{\n\t\tisInWeb = false;\n\t\td *= 0.25;',
  '\tif (isInWeb && AdminHooks::onFor(AdminHooks::NoSlowDown, this))\n\t\tisInWeb = false;\n\tif (isInWeb)\n\t{\n\t\tisInWeb = false;\n\t\td *= 0.25;'),
])
edit(N+'EntityLiving.cpp', [
 ('void EntityLiving::jump()\n{\n\tmotionY = 0.41999998688697815;\n',
  'void EntityLiving::jump()\n{\n\tmotionY = 0.41999998688697815;\n\tif (AdminHooks::onFor(AdminHooks::HighJump, this))\n\t\tmotionY *= (double)AdminHooks::highJumpFactor();\n'),
 ('void EntityLiving::knockBack(Entity *entity, int_t i, double d, double d1)\n{\n',
  'void EntityLiving::knockBack(Entity *entity, int_t i, double d, double d1)\n{\n\tif (AdminHooks::onFor(AdminHooks::NoFlinch, this))\n\t\treturn;\n'),
])
edit(N+'EntityPlayer.cpp', [
 ('void EntityPlayer::swingItem()\n{\n',
  'void EntityPlayer::swingItem()\n{\n\tif (AdminHooks::onFor(AdminHooks::NoSwing, this))\n\t\treturn;\n'),
])
edit(N+'EntityRenderer.cpp', [
 ('        const float gamma = mc->gameSettings->ofBrightness;\n',
  '        const float gamma = (AdminHooks::on(AdminHooks::FullBright) || AdminHooks::on(AdminHooks::CaveFinder))\n'
  '            ? 8.0f : mc->gameSettings->ofBrightness; // admin mode FullBright, as Nodus\n'),
 ('    if (hurtTime < 0.0f)\n    {\n        return;\n    }\n    \n    hurtTime /= entityliving->maxHurtTime;',
  '    if (hurtTime < 0.0f)\n    {\n        return;\n    }\n    if (AdminHooks::onFor(AdminHooks::NoFlinch, entityliving))\n        return;\n    \n    hurtTime /= entityliving->maxHurtTime;'),
 ('void EntityRenderer::renderRainSnow(float partialTicks)\n{\n',
  'void EntityRenderer::renderRainSnow(float partialTicks)\n{\n    if (AdminHooks::on(AdminHooks::NoRender))\n        return;\n'),
 ('void EntityRenderer::addRainParticles()\n{\n',
  'void EntityRenderer::addRainParticles()\n{\n    if (AdminHooks::on(AdminHooks::NoRender))\n        return;\n'),
 ('        XBOX_FRAME_SLOT_BEGIN(xboxWeather);\n        renderRainSnow(partialTicks);',
  '        if (AdminHooks::g_any)\n            AdminHooks::renderWorldOverlay(partialTicks);\n        XBOX_FRAME_SLOT_BEGIN(xboxWeather);\n        renderRainSnow(partialTicks);'),
])
edit(N+'EffectRenderer.cpp', [
 ('void EffectRenderer::addEffect(EntityFX *entityfx)\n{\n\tif (entityfx == nullptr)\n\t\treturn;\n',
  'void EffectRenderer::addEffect(EntityFX *entityfx)\n{\n\tif (entityfx == nullptr)\n\t\treturn;\n'
  '\tif (AdminHooks::on(AdminHooks::NoRender))\n\t{\n\t\tdelete entityfx;\n\t\treturn;\n\t}\n'),
])
edit(N+'RenderLiving.cpp', [
 ('void RenderLiving::doRenderLiving(EntityLiving* entityliving, double d, double d1, double d2, float f, float f1) {\n    renderPushMatrix();\n',
  'void RenderLiving::doRenderLiving(EntityLiving* entityliving, double d, double d1, double d2, float f, float f1) {\n'
  '    const bool adminWallHack = AdminHooks::on(AdminHooks::WallHack) && !AdminHooks::isLocal(entityliving);\n'
  '    if (adminWallHack)\n        renderDisable(RenderCapability::DepthTest);\n    renderPushMatrix();\n'),
 ('    renderPopMatrix();\n    resetLivingRenderGLState();\n    passSpecialRender(entityliving, d, d1, d2);',
  '    renderPopMatrix();\n    if (adminWallHack)\n        renderEnable(RenderCapability::DepthTest);\n    resetLivingRenderGLState();\n    passSpecialRender(entityliving, d, d1, d2);'),
 ('void RenderLiving::renderLivingLabel(EntityLiving* entityliving, std::string s, double d, double d1, double d2, int i) {\n',
  'void RenderLiving::renderLivingLabel(EntityLiving* entityliving, std::string s, double d, double d1, double d2, int i) {\n'
  '    if (AdminHooks::on(AdminHooks::DisableNametags)) return;\n'),
])
edit(N+'RenderItem.cpp', [
 ('void RenderItem::doRender(Entity* entity, double d, double d1, double d2, float f, float f1) {\n',
  'void RenderItem::doRender(Entity* entity, double d, double d1, double d2, float f, float f1) {\n'
  '    if (AdminHooks::on(AdminHooks::NoRender) && AdminHooks::noRenderItems()) return;\n'),
])
edit(N+'BlockFluid.cpp', [
 ('AxisAlignedBB *BlockFluid::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)\n{\n\treturn nullptr;',
  'AxisAlignedBB *BlockFluid::getCollisionBoundingBoxFromPool(World *world, int_t i, int_t j, int_t k)\n{\n'
  '\t// Admin mode "Water": the local player walks on water (sneak to sink), as Nodus.\n'
  '\tif (AdminHooks::on(AdminHooks::Water) && blockMaterial == Material::water)\n\t{\n'
  '\t\tEntity *e = AdminHooks::g_collidingEntity;\n'
  '\t\tif (AdminHooks::isLocal(e) && !e->isSneaking() && e->boundingBox->minY >= (double)j + 0.6)\n'
  '\t\t\treturn AxisAlignedBB::getBoundingBoxFromPool((double)i, (double)j, (double)k, (double)i + 1.0, (double)j + 0.7, (double)k + 1.0);\n'
  '\t}\n\treturn nullptr;'),
])
edit(N+'World.cpp', [
 ('std::vector<AxisAlignedBB*> &World::getCollidingBoundingBoxes(Entity* entity, AxisAlignedBB* aabb)\n{\n    collidingBoundingBoxes.clear();\n',
  'std::vector<AxisAlignedBB*> &World::getCollidingBoundingBoxes(Entity* entity, AxisAlignedBB* aabb)\n{\n    collidingBoundingBoxes.clear();\n    AdminHooks::g_collidingEntity = entity;\n'),
 ('#endif\n    \n    double d = 0.25;\n    const auto& list = getEntitiesWithinAABBExcludingEntity(entity, aabb->expand(d, d, d));',
  '#endif\n    AdminHooks::g_collidingEntity = nullptr;\n    \n    double d = 0.25;\n    const auto& list = getEntitiesWithinAABBExcludingEntity(entity, aabb->expand(d, d, d));'),
])
edit(N+'PlayerControllerSP.cpp', [
 ('\t\tif (curBlockDamage >= 1.0f)\n\t\t{\n\t\t\tsendBlockRemoved(i, j, k, l);\n\t\t\tcurBlockDamage = 0.0f;\n\t\t\tprevBlockDamage = 0.0f;\n\t\t\tfield_1069_h = 0.0f;\n\t\t\tblockHitWait = 5;',
  '\t\tconst bool adminFast = AdminHooks::on(AdminHooks::FastBreak);\n'
  '\t\tif (adminFast && AdminHooks::fastBreakInstant())\n\t\t\tcurBlockDamage = 1.0f;\n'
  '\t\tif (curBlockDamage >= (adminFast ? 0.55f : 1.0f))\n\t\t{\n\t\t\tsendBlockRemoved(i, j, k, l);\n\t\t\tcurBlockDamage = 0.0f;\n\t\t\tprevBlockDamage = 0.0f;\n\t\t\tfield_1069_h = 0.0f;\n\t\t\tblockHitWait = adminFast ? 0 : 5;'),
])
edit('client/Minecraft.cpp', [
 ('            if (playerController->sendPlaceBlock(thePlayer, theWorld, itemstack1, j, k, lv, i1))\n            {\n                flag = false;\n',
  '            if (playerController->sendPlaceBlock(thePlayer, theWorld, itemstack1, j, k, lv, i1))\n            {\n                flag = false;\n'
  '                if (AdminHooks::on(AdminHooks::Build))\n                    AdminHooks::onBlockPlaced(j, k, lv, i1);\n'),
])
