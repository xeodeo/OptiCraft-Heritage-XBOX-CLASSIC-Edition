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
	settings.keyBindForward->keyCode = XBOX_KEY_LSTICK_UP;
	settings.keyBindLeft->keyCode = XBOX_KEY_LSTICK_LEFT;
	settings.keyBindBack->keyCode = XBOX_KEY_LSTICK_DOWN;
	settings.keyBindRight->keyCode = XBOX_KEY_LSTICK_RIGHT;
	settings.keyBindJump->keyCode = XBOX_KEY_A;
	settings.keyBindInventory->keyCode = XBOX_KEY_Y;
	settings.keyBindDrop->keyCode = XBOX_KEY_B;
	settings.keyBindSneak->keyCode = XBOX_KEY_LEFT_THUMB;
	settings.keyBindAttack->keyCode = XBOX_KEY_RIGHT_TRIGGER;
	settings.keyBindUseItem->keyCode = XBOX_KEY_LEFT_TRIGGER;
	if (settings.keyBindCrafting != nullptr)
		settings.keyBindCrafting->keyCode = XBOX_KEY_X;
	// The D-pad does only what the player binds to it (user, 03-11): hotbar
	// stays on White/Black, up/down fly in creative, left/right are free.
	if (settings.keyBindPrevItem != nullptr)
		settings.keyBindPrevItem->keyCode = XBOX_KEY_WHITE;
	if (settings.keyBindNextItem != nullptr)
		settings.keyBindNextItem->keyCode = XBOX_KEY_BLACK;
	if (settings.keyBindTogglePerspective != nullptr)
		settings.keyBindTogglePerspective->keyCode = XBOX_KEY_RIGHT_THUMB;
	if (settings.keyBindFlyUp != nullptr)
		settings.keyBindFlyUp->keyCode = XBOX_KEY_DPAD_UP;
	if (settings.keyBindFlyDown != nullptr)
		settings.keyBindFlyDown->keyCode = XBOX_KEY_DPAD_DOWN;
	if (settings.keyBindChat != nullptr)
		settings.keyBindChat->keyCode = lwjgl::Keyboard::KEY_NONE;
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
	// Saves from before the D-pad split walked with the D-pad codes (or a
	// keyboard code): move them to the stick so the stick walks again.
	if (settings.keyBindForward->keyCode == XBOX_KEY_DPAD_UP || settings.keyBindForward->keyCode == 17) settings.keyBindForward->keyCode = XBOX_KEY_LSTICK_UP;
	if (settings.keyBindLeft->keyCode == XBOX_KEY_DPAD_LEFT || settings.keyBindLeft->keyCode == 30) settings.keyBindLeft->keyCode = XBOX_KEY_LSTICK_LEFT;
	if (settings.keyBindBack->keyCode == XBOX_KEY_DPAD_DOWN || settings.keyBindBack->keyCode == 31) settings.keyBindBack->keyCode = XBOX_KEY_LSTICK_DOWN;
	if (settings.keyBindRight->keyCode == XBOX_KEY_DPAD_RIGHT || settings.keyBindRight->keyCode == 32) settings.keyBindRight->keyCode = XBOX_KEY_LSTICK_RIGHT;
	migrateKey(settings.keyBindJump, XBOX_KEY_A);
	migrateKey(settings.keyBindInventory, XBOX_KEY_Y);
	migrateKey(settings.keyBindDrop, XBOX_KEY_B);
	migrateKey(settings.keyBindSneak, XBOX_KEY_LEFT_THUMB);
	migrateKey(settings.keyBindAttack, XBOX_KEY_RIGHT_TRIGGER);
	migrateKey(settings.keyBindUseItem, XBOX_KEY_LEFT_TRIGGER);
	if (settings.keyBindCrafting != nullptr)
		migrateKey(settings.keyBindCrafting, XBOX_KEY_X);
	migrateKey(settings.keyBindPrevItem, XBOX_KEY_WHITE);
	migrateKey(settings.keyBindNextItem, XBOX_KEY_BLACK);
	migrateKey(settings.keyBindTogglePerspective, XBOX_KEY_RIGHT_THUMB);
	migrateKey(settings.keyBindFlyUp, XBOX_KEY_DPAD_UP);
	migrateKey(settings.keyBindFlyDown, XBOX_KEY_DPAD_DOWN);
	// Chat has no default button on the pad: a keyboard code (T) means unbound.
	if (settings.keyBindChat != nullptr && settings.keyBindChat->keyCode < lwjgl::Keyboard::KEY_MAX)
		settings.keyBindChat->keyCode = lwjgl::Keyboard::KEY_NONE;
}

void platformGameSettingsSyncControllerBindings(const GameSettings&) {}
void platformGameSettingsAddKnownKeys(std::unordered_set<std::string>&) {}
void platformGameSettingsWriteOptions(const GameSettings&, std::ostream&) {}
void platformGameSettingsApplyLegacyCrafting(GameSettings&) {}

