#include "platform/GameSettingsBackend.h"

#include <algorithm>
#include <ostream>
#include "lwjgl/Keyboard.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/KeyBinding.h"
#include "platform/PlatformTuning.h"
#include "xbox/input/XboxPadKeyCodes.h"

namespace
{
// Controller defaults (same slots as the PS2 layout, Xbox button names):
// stick/D-pad move, A jump, Y inventory, B drop, left stick click sneak.
// Attack/use (RT/LT), hotbar (White/Black), pause and view are synthesized
// directly by src/xbox/input/XboxInput.cpp.
void applyDefaultBindings(GameSettings& settings)
{
	settings.keyBindForward->keyCode = XBOX_KEY_DPAD_UP;
	settings.keyBindLeft->keyCode = XBOX_KEY_DPAD_LEFT;
	settings.keyBindBack->keyCode = XBOX_KEY_DPAD_DOWN;
	settings.keyBindRight->keyCode = XBOX_KEY_DPAD_RIGHT;
	settings.keyBindJump->keyCode = XBOX_KEY_A;
	settings.keyBindInventory->keyCode = XBOX_KEY_Y;
	settings.keyBindDrop->keyCode = XBOX_KEY_B;
	settings.keyBindSneak->keyCode = XBOX_KEY_LEFT_THUMB;
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
	migrateKey(settings.keyBindForward, XBOX_KEY_DPAD_UP);
	migrateKey(settings.keyBindLeft, XBOX_KEY_DPAD_LEFT);
	migrateKey(settings.keyBindBack, XBOX_KEY_DPAD_DOWN);
	migrateKey(settings.keyBindRight, XBOX_KEY_DPAD_RIGHT);
	migrateKey(settings.keyBindJump, XBOX_KEY_A);
	migrateKey(settings.keyBindInventory, XBOX_KEY_Y);
	migrateKey(settings.keyBindDrop, XBOX_KEY_B);
	migrateKey(settings.keyBindSneak, XBOX_KEY_LEFT_THUMB);
}

void platformGameSettingsSyncControllerBindings(const GameSettings&) {}
void platformGameSettingsAddKnownKeys(std::unordered_set<std::string>&) {}
void platformGameSettingsWriteOptions(const GameSettings&, std::ostream&) {}
