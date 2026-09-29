#include "platform/GameSettingsBackend.h"

#include <ostream>

#include "platform/PlatformConfig.h"
#include "platform/PlatformTuning.h"

#include "lwjgl/Keyboard.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/KeyBinding.h"
#include "pc/render/PcRenderBackend.h"

void platformGameSettingsApplyLegacyCrafting(GameSettings& settings)
{
	if (settings.keyBindCrafting != nullptr)
	{
		if (settings.legacyCrafting)
		{
			if (settings.keyBindCrafting->keyCode == 0)
				settings.keyBindCrafting->keyCode = lwjgl::Keyboard::KEY_C;
		}
		else
		{
			settings.keyBindCrafting->keyCode = 0;
		}
	}
}

void platformGameSettingsInitialize(GameSettings& settings)
{
	settings.renderBackend = static_cast<int_t>(pcRenderBackendGetRequested());
	platformGameSettingsApplyLegacyCrafting(settings);
}
void platformGameSettingsResetControlBindings(GameSettings&) {}
int_t platformGameSettingsDefaultChunkUpdates() { return 1; }
int_t platformGameSettingsDefaultConnectedTextures()
{
#if PLATFORM_PC_LEGACY
	return 3;
#else
	return 2;
#endif
}

int_t platformGameSettingsCycleRenderDistance(int_t current, int_t delta)
{
#if PLATFORM_PC_LEGACY
	const int_t index = current <= 2 ? 0 : 1;
	int_t next = (index + delta) % 2;
	if (next < 0)
		next += 2;
	return 2 + next;
#else
	return (current + delta) & 3;
#endif
}

int_t platformGameSettingsClampRenderDistance(int_t value)
{
#if PLATFORM_PC_LEGACY
	return value < 2 ? 2 : (value > 3 ? 3 : value);
#else
	return value;
#endif
}

int_t platformGameSettingsClampFineRenderDistance(int_t value)
{
#if PLATFORM_PC_LEGACY
	const int_t maxDistance = PLATFORM_VISIBLE_CHUNK_RADIUS * 16;
	return value < 32 ? 32 : (value > maxDistance ? maxDistance : value);
#else
	return value;
#endif
}

void platformGameSettingsUpdateRenderDistanceFromFine(int_t fineDistance, int_t& renderDistance)
{
#if PLATFORM_PC_LEGACY
	fineDistance = platformGameSettingsClampFineRenderDistance(fineDistance);
#endif
	renderDistance = 3;
	if (fineDistance > 32) renderDistance = 2;
	if (fineDistance > 64) renderDistance = 1;
	if (fineDistance > 128) renderDistance = 0;
}

bool platformGameSettingsAnaglyphValue(bool, bool requested) { return requested; }
bool platformGameSettingsLoadOption(GameSettings& settings, const std::string& key, const std::string& value)
{
	if (key != "renderBackend")
		return false;

	const PcRenderBackendType backend = pcRenderBackendFromString(value);
	settings.renderBackend = static_cast<int_t>(backend);
	pcRenderBackendSetRequested(backend);
	return true;
}

void platformGameSettingsFinalizeLoad(GameSettings& settings)
{
	PcRenderBackendType backend = settings.renderBackend == static_cast<int_t>(PcRenderBackendType::Direct3D9)
		? PcRenderBackendType::Direct3D9
		: PcRenderBackendType::OpenGL;
	if (!pcRenderBackendIsSupported(backend))
		backend = PcRenderBackendType::OpenGL;
	settings.renderBackend = static_cast<int_t>(backend);
	pcRenderBackendSetRequested(backend);
	platformGameSettingsApplyLegacyCrafting(settings);
}
void platformGameSettingsSyncControllerBindings(const GameSettings&) {}
void platformGameSettingsAddKnownKeys(std::unordered_set<std::string>& knownKeys)
{
	knownKeys.insert("renderBackend");
}

void platformGameSettingsWriteOptions(const GameSettings& settings, std::ostream& output)
{
	const PcRenderBackendType backend = settings.renderBackend == static_cast<int_t>(PcRenderBackendType::Direct3D9)
		? PcRenderBackendType::Direct3D9
		: PcRenderBackendType::OpenGL;
	output << "renderBackend:" << pcRenderBackendConfigName(backend) << "\n";
}
