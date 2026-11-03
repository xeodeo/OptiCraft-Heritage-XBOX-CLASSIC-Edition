#include "platform/GameSettingsBackend.h"

#include <algorithm>
#include "lwjgl/Keyboard.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/KeyBinding.h"
#include "platform/PlatformTuning.h"
#include "ps2/input/Ps2PadKeyCodes.h"

namespace
{
void migrateKey(KeyBinding* binding, int_t fallback)
{
	if (binding->keyCode < lwjgl::Keyboard::KEY_MAX)
		binding->keyCode = fallback;
}
}

void platformGameSettingsApplyLegacyCrafting(GameSettings& settings)
{
	if (settings.xboxStyleCrafting)
	{
		settings.keyBindInventory->keyCode = PS2_KEY_TRIANGLE;
		if (settings.keyBindCrafting != nullptr)
			settings.keyBindCrafting->keyCode = PS2_KEY_SQUARE;
		settings.keyBindDrop->keyCode = PS2_KEY_CIRCLE;
	}
	else
	{
		settings.keyBindInventory->keyCode = PS2_KEY_SQUARE;
		if (settings.keyBindCrafting != nullptr)
			settings.keyBindCrafting->keyCode = 0;
		settings.keyBindDrop->keyCode = PS2_KEY_CIRCLE;
	}
}

void platformGameSettingsInitialize(GameSettings& settings)
{
	settings.keyBindForward->keyCode = 17;
	settings.keyBindLeft->keyCode = 30;
	settings.keyBindBack->keyCode = 31;
	settings.keyBindRight->keyCode = 32;
	settings.keyBindJump->keyCode = PS2_KEY_CROSS;
	settings.keyBindSneak->keyCode = PS2_KEY_L3;
	settings.keyBindAttack->keyCode = PS2_KEY_R2;
	settings.keyBindUseItem->keyCode = PS2_KEY_L2;
	platformGameSettingsApplyLegacyCrafting(settings);
}

void platformGameSettingsResetControlBindings(GameSettings& settings)
{
	settings.keyBindForward->keyCode = 17;
	settings.keyBindLeft->keyCode = 30;
	settings.keyBindBack->keyCode = 31;
	settings.keyBindRight->keyCode = 32;
	settings.keyBindJump->keyCode = PS2_KEY_CROSS;
	settings.keyBindSneak->keyCode = PS2_KEY_L3;
	settings.keyBindAttack->keyCode = PS2_KEY_R2;
	settings.keyBindUseItem->keyCode = PS2_KEY_L2;
	platformGameSettingsApplyLegacyCrafting(settings);
}

int_t platformGameSettingsDefaultChunkUpdates() { return (int_t)PLATFORM_MAX_RENDERER_UPDATES_PER_FRAME; }
int_t platformGameSettingsDefaultConnectedTextures() { return 3; }
int_t platformGameSettingsCycleRenderDistance(int_t, int_t) { return PLATFORM_DEFAULT_RENDER_DISTANCE; }
int_t platformGameSettingsClampRenderDistance(int_t) { return PLATFORM_DEFAULT_RENDER_DISTANCE; }
int_t platformGameSettingsClampFineRenderDistance(int_t value)
{
	return value < 32 ? 32 : (value > PLATFORM_VISIBLE_CHUNK_RADIUS * 16 ? PLATFORM_VISIBLE_CHUNK_RADIUS * 16 : value);
}
void platformGameSettingsUpdateRenderDistanceFromFine(int_t, int_t&) {}
bool platformGameSettingsAnaglyphValue(bool, bool requested) { return requested; }
bool platformGameSettingsLoadOption(GameSettings&, const std::string&, const std::string&) { return false; }

void platformGameSettingsFinalizeLoad(GameSettings& settings)
{
	settings.ofChunkUpdates = std::max(settings.ofChunkUpdates, (int_t)PLATFORM_MAX_RENDERER_UPDATES_PER_FRAME);
	if (settings.keyBindForward->keyCode == PS2_KEY_DPAD_UP) settings.keyBindForward->keyCode = 17;
	if (settings.keyBindLeft->keyCode == PS2_KEY_DPAD_LEFT) settings.keyBindLeft->keyCode = 30;
	if (settings.keyBindBack->keyCode == PS2_KEY_DPAD_DOWN) settings.keyBindBack->keyCode = 31;
	if (settings.keyBindRight->keyCode == PS2_KEY_DPAD_RIGHT) settings.keyBindRight->keyCode = 32;
	migrateKey(settings.keyBindJump, PS2_KEY_CROSS);
	migrateKey(settings.keyBindInventory, PS2_KEY_TRIANGLE);
	migrateKey(settings.keyBindDrop, PS2_KEY_CIRCLE);
	migrateKey(settings.keyBindSneak, PS2_KEY_L3);
	migrateKey(settings.keyBindAttack, PS2_KEY_R2);
	migrateKey(settings.keyBindUseItem, PS2_KEY_L2);
}

void platformGameSettingsSyncControllerBindings(const GameSettings&) {}
void platformGameSettingsAddKnownKeys(std::unordered_set<std::string>&) {}
void platformGameSettingsWriteOptions(const GameSettings&, std::ostream&) {}
