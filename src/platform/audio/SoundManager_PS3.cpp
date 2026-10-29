// SoundManager_PS3.cpp -- Minecraft sound on the PS3.
//
// Silent for now: the first goal of the port is the menu on screen. The real
// backend will follow SoundManager_XBOX.cpp / SoundManager_WII.cpp (stb_vorbis
// decode, bounded SFX cache, one streamed music voice) over cellAudio's ports.
#include "net/minecraft/src/SoundManager.h"

#include "platform/Log.h"

void *SoundManager::sndSystem = nullptr;
bool  SoundManager::loaded    = false;

SoundManager::SoundManager()
    : soundPoolSounds(), soundPoolStreaming(), soundPoolMusic(), soundVolume(0),
      options(nullptr), rand(), ticksBeforeMusic(0)
{
}

void SoundManager::loadSoundSettings(GameSettings *gamesettings) { options = gamesettings; loaded = false; }
void SoundManager::onSoundOptionsChanged() {}
void SoundManager::closeMinecraft() { loaded = false; }
void SoundManager::addSound(const jstring &s, const std::string &file) { soundPoolSounds.addSound(s, file); }
void SoundManager::addStreaming(const jstring &s, const std::string &file) { soundPoolStreaming.addSound(s, file); }
void SoundManager::addMusic(const jstring &s, const std::string &file) { soundPoolMusic.addSound(s, file); }
void SoundManager::playRandomMusicIfReady() {}
bool SoundManager::playMusicFileNow(const std::string &) { return false; }
void SoundManager::setListenerPosition(EntityLiving *, float) {}
void SoundManager::playStreaming(const jstring &, float, float, float, float, float) {}
void SoundManager::playSound(const jstring &, float, float, float, float, float) {}
void SoundManager::playSoundFX(const jstring &, float, float) {}
void SoundManager::tryToSetLibraryAndCodecs() {}
