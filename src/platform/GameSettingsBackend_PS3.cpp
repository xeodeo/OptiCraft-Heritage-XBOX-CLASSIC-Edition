#include "platform/GameSettingsBackend.h"

#include <algorithm>
#include <ostream>
#include "lwjgl/Keyboard.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/KeyBinding.h"
#include "platform/PlatformTuning.h"
#include "ps3/input/Ps3PadKeyCodes.h"

namespace
{
// Controller defaults (same slots as the Xbox layout, DualShock 3 names):
// stick/D-pad move, Cross jump, Triangle inventory, Circle drop, L3 sneak.
// Attack/use (R2/L2), hotbar (L1/R1), pause and view are synthesized directly
// by the PS3 input layer (src/ps3/input).
void applyDefaultBindings(GameSettings& settings)
{
	settings.keyBindForward->keyCode = PS3_KEY_DPAD_UP;
	settings.keyBindLeft->keyCode = PS3_KEY_DPAD_LEFT;
	settings.keyBindBack->keyCode = PS3_KEY_DPAD_DOWN;
	settings.keyBindRight->keyCode = PS3_KEY_DPAD_RIGHT;
	settings.keyBindJump->keyCode = PS3_KEY_CROSS;
	settings.keyBindInventory->keyCode = PS3_KEY_TRIANGLE;
	settings.keyBindDrop->keyCode = PS3_KEY_CIRCLE;
	settings.keyBindSneak->keyCode = PS3_KEY_L3;
}

// An options.txt written with keyboard codes (or by another platform) gets the
// controller defaults back, like the PS2 migration.
void migrateKey(KeyBinding* binding, int_t fallback)
{
	if (binding->keyCode < lwjgl::Keyboard::KEY_MAX)
		binding->keyCode = fallback;
}
}

void platformGameSettingsInitialize(GameSettings& settings)
{
	applyDefaultBindings(settings);
}

void platformGameSettingsResetControlBindings(GameSettings& settings)
{
	applyDefaultBindings(settings);
}

int_t platformGameSettingsDefaultChunkUpdates() { return (int_t)PLATFORM_MAX_RENDERER_UPDATES_PER_FRAME; }
int_t platformGameSettingsDefaultConnectedTextures() { return 2; }

int_t platformGameSettingsCycleRenderDistance(int_t current, int_t delta)
{
	const int_t span = 3 - (int_t)PLATFORM_DEFAULT_RENDER_DISTANCE + 1;
	current += delta;
	while (current > 3) current -= span;
	while (current < (int_t)PLATFORM_DEFAULT_RENDER_DISTANCE) current += span;
	return current;
}

int_t platformGameSettingsClampRenderDistance(int_t value)
{
	if (value < (int_t)PLATFORM_DEFAULT_RENDER_DISTANCE) return (int_t)PLATFORM_DEFAULT_RENDER_DISTANCE;
	return value > 3 ? 3 : value;
}

int_t platformGameSettingsClampFineRenderDistance(int_t value)
{
	return value < 32 ? 32 : (value > PLATFORM_VISIBLE_CHUNK_RADIUS * 16 ? PLATFORM_VISIBLE_CHUNK_RADIUS * 16 : value);
}

void platformGameSettingsUpdateRenderDistanceFromFine(int_t, int_t&) {}
bool platformGameSettingsAnaglyphValue(bool, bool) { return false; }
bool platformGameSettingsLoadOption(GameSettings&, const std::string&, const std::string&) { return false; }

void platformGameSettingsFinalizeLoad(GameSettings& settings)
{
	if (settings.renderDistance < 1 || settings.renderDistance > 3)
		settings.renderDistance = PLATFORM_DEFAULT_RENDER_DISTANCE;
	settings.ofChunkUpdates = std::max(settings.ofChunkUpdates, (int_t)PLATFORM_MAX_RENDERER_UPDATES_PER_FRAME);
	migrateKey(settings.keyBindForward, PS3_KEY_DPAD_UP);
	migrateKey(settings.keyBindLeft, PS3_KEY_DPAD_LEFT);
	migrateKey(settings.keyBindBack, PS3_KEY_DPAD_DOWN);
	migrateKey(settings.keyBindRight, PS3_KEY_DPAD_RIGHT);
	migrateKey(settings.keyBindJump, PS3_KEY_CROSS);
	migrateKey(settings.keyBindInventory, PS3_KEY_TRIANGLE);
	migrateKey(settings.keyBindDrop, PS3_KEY_CIRCLE);
	migrateKey(settings.keyBindSneak, PS3_KEY_L3);
}

void platformGameSettingsSyncControllerBindings(const GameSettings&) {}
void platformGameSettingsAddKnownKeys(std::unordered_set<std::string>&) {}
void platformGameSettingsWriteOptions(const GameSettings&, std::ostream&) {}
void platformGameSettingsApplyLegacyCrafting(GameSettings&) {}

