// SoundManager_XBOX.cpp — Minecraft sound over the Xbox DirectSound / MCPX APU.
//
// The APU mixes every voice in hardware (and, with the Dolby Digital option
// on, DirectSound encodes a 5.1 mix to AC-3 itself); the CPU only decodes
// Vorbis. There is no separate audio RAM, so memory is kept small:
//
//  * One-shot sounds are decoded once to 16-bit PCM into a bounded LRU cache
//    (kSfxCacheBudget). Voices play straight out of that memory through
//    SetBufferData -- no second copy per voice. The APU reads it by physical
//    address with its own DMA, so the cache lives in physically contiguous,
//    write-combined memory (XPhysicalAlloc); ordinary heap memory is neither
//    contiguous nor guaranteed out of the CPU cache, and on real hardware the
//    APU then plays garbage and DirectSound hangs the console (xemu does not
//    care). An entry is only evicted when no voice is playing it, and is
//    detached from its voice before it is freed.
//  * Music and jukebox records stream through one small looping buffer that a
//    worker thread refills from stb_vorbis.
//
// Same structure and policies as SoundManager_WII.cpp (pools, attenuation,
// music timer); XDK DirectSound is not thread-safe, so every call into it is
// made under s_dsLock.
#include "net/minecraft/src/SoundManager.h"

#if defined(NO_SOUND)

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

#else

#include "xbox/XboxXtl.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "pc/external/stb_vorbis.h"
#include "platform/audio/VorbisAssetOpen.h"

#include "net/minecraft/src/EntityLiving.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/SoundPoolEntry.h"
#include "platform/Log.h"
#include "platform/audio/AudioAssetFormat.h"
#include "platform/audio/AudioSpatialization.h"

void *SoundManager::sndSystem = nullptr;
bool  SoundManager::loaded    = false;

namespace
{
AudioListenerState s_listener;
LPDIRECTSOUND s_ds = nullptr;
bool s_dolbyActive = false;   // output the current DirectSound was created with
CRITICAL_SECTION s_dsLock;
bool s_dsLockReady = false;

struct DsLock
{
    DsLock() { EnterCriticalSection(&s_dsLock); }
    ~DsLock() { LeaveCriticalSection(&s_dsLock); }
};

LONG toDsVolume(float volume)
{
    if (volume <= 0.0001f)
        return DSBVOLUME_MIN;
    if (volume >= 1.0f)
        return 0;
    const LONG v = static_cast<LONG>(2000.0f * std::log10(volume));
    return v < DSBVOLUME_MIN ? DSBVOLUME_MIN : v;
}

// ---- One-shot sounds -------------------------------------------------------
constexpr size_t kSfxCacheBudget = 1024 * 1024;   // decoded PCM kept resident
constexpr size_t kSfxMaxSample = 256 * 1024;      // longer clips are skipped
constexpr int kSfxVoices = 16;

struct SfxSample
{
    short *pcm = nullptr;
    DWORD bytes = 0;
    int channels = 1;
    int sampleRate = 44100;
    DWORD lastUsed = 0;
};

struct SfxVoice
{
    LPDIRECTSOUNDBUFFER buffer = nullptr;
    const SfxSample *sample = nullptr;
    DWORD startedAt = 0;
};

std::unordered_map<std::string, SfxSample> s_sfxCache;
std::unordered_set<std::string> s_sfxRejected;
size_t s_sfxCacheBytes = 0;
SfxVoice s_voices[kSfxVoices];

bool voicePlaying(const SfxVoice &voice)
{
    if (voice.buffer == nullptr || voice.sample == nullptr)
        return false;
    DWORD status = 0;
    voice.buffer->GetStatus(&status);
    return (status & DSBSTATUS_PLAYING) != 0;
}

bool sampleInUse(const SfxSample *sample)
{
    for (const SfxVoice &voice : s_voices)
        if (voice.sample == sample && voicePlaying(voice))
            return true;
    return false;
}

bool evictOneSfx()
{
    auto victim = s_sfxCache.end();
    for (auto it = s_sfxCache.begin(); it != s_sfxCache.end(); ++it)
    {
        if (sampleInUse(&it->second))
            continue;
        if (victim == s_sfxCache.end() || it->second.lastUsed < victim->second.lastUsed)
            victim = it;
    }
    if (victim == s_sfxCache.end())
        return false;
    for (SfxVoice &voice : s_voices)
    {
        if (voice.sample == &victim->second)
        {
            voice.buffer->Stop();
            voice.buffer->SetBufferData(nullptr, 0);
            voice.sample = nullptr;
        }
    }
    s_sfxCacheBytes -= victim->second.bytes;
    XPhysicalFree(victim->second.pcm);
    s_sfxCache.erase(victim);
    return true;
}

const SfxSample *getSfxSample(const std::string &path)
{
    auto it = s_sfxCache.find(path);
    if (it != s_sfxCache.end())
    {
        it->second.lastUsed = GetTickCount();
        return &it->second;
    }
    if (s_sfxRejected.count(path) != 0)
        return nullptr;

    int channels = 0, rate = 0;
    short *pcm = nullptr;
    const int frames = platformDecodeVorbis(path, &channels, &rate, &pcm);
    if (frames <= 0 || pcm == nullptr)
    {
        s_sfxRejected.insert(path);
        return nullptr;
    }
    const DWORD bytes = static_cast<DWORD>(frames) * static_cast<DWORD>(channels) * sizeof(short);
    if (bytes > kSfxMaxSample || channels > 2)
    {
        std::free(pcm);
        s_sfxRejected.insert(path);
        return nullptr;
    }
    while (s_sfxCacheBytes + bytes > kSfxCacheBudget && evictOneSfx())
    {
    }
    if (s_sfxCacheBytes + bytes > kSfxCacheBudget)
    {
        std::free(pcm);   // everything resident is playing; drop this one
        return nullptr;
    }
    // Memory the APU can DMA from: contiguous, write-combined (see top).
    short *apuPcm = static_cast<short *>(XPhysicalAlloc(bytes, MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_WRITECOMBINE));
    if (apuPcm == nullptr)
    {
        std::free(pcm);
        return nullptr;
    }
    std::memcpy(apuPcm, pcm, bytes);
    std::free(pcm);
    SfxSample &sample = s_sfxCache[path];
    sample.pcm = apuPcm;
    sample.bytes = bytes;
    sample.channels = channels;
    sample.sampleRate = rate;
    sample.lastUsed = GetTickCount();
    s_sfxCacheBytes += bytes;
    return &sample;
}

void playSfx(const std::string &path, float volume, float pitch)
{
    if (volume <= 0.0f)
        return;
    DsLock lock;
    const SfxSample *sample = getSfxSample(path);
    if (sample == nullptr)
        return;

    // Free voice, else the one that started longest ago.
    SfxVoice *voice = nullptr;
    for (SfxVoice &candidate : s_voices)
    {
        if (candidate.buffer == nullptr)
            continue;
        if (!voicePlaying(candidate)) { voice = &candidate; break; }
        if (voice == nullptr || candidate.startedAt < voice->startedAt)
            voice = &candidate;
    }
    if (voice == nullptr)
        return;

    WAVEFORMATEX format;
    XAudioCreatePcmFormat(static_cast<WORD>(sample->channels), static_cast<DWORD>(sample->sampleRate), 16, &format);
    voice->buffer->Stop();
    voice->buffer->SetFormat(&format);
    voice->buffer->SetBufferData(sample->pcm, sample->bytes);
    voice->buffer->SetCurrentPosition(0);
    float frequency = static_cast<float>(sample->sampleRate) * (pitch > 0.0f ? pitch : 1.0f);
    frequency = std::max<float>(DSBFREQUENCY_MIN, std::min<float>(DSBFREQUENCY_MAX, frequency));
    voice->buffer->SetFrequency(static_cast<DWORD>(frequency));
    voice->buffer->SetVolume(toDsVolume(volume));
    voice->buffer->Play(0, 0, 0);
    voice->sample = sample;
    voice->startedAt = GetTickCount();
}

void clearSfx()
{
    DsLock lock;
    for (SfxVoice &voice : s_voices)
    {
        if (voice.buffer != nullptr)
        {
            voice.buffer->Stop();
            voice.buffer->SetBufferData(nullptr, 0);
        }
        voice.sample = nullptr;
    }
    for (auto &entry : s_sfxCache)
        XPhysicalFree(entry.second.pcm);
    s_sfxCache.clear();
    s_sfxCacheBytes = 0;
}

// ---- Streaming (music, records) --------------------------------------------
enum class StreamKind { None, Music, Streaming };

constexpr DWORD kStreamBytes = 64 * 1024;       // looping ring, stereo 16-bit
constexpr DWORD kStreamHalf = kStreamBytes / 2;

LPDIRECTSOUNDBUFFER s_stream = nullptr;
HANDLE s_streamThread = nullptr;
volatile LONG s_streamStop = 0;
volatile StreamKind s_streamKind = StreamKind::None;
std::string s_streamPath;
float s_streamVolume = 1.0f;

// Decodes the next half of the ring. Returns false once the stream (and the
// silence that lets its tail play out) is finished.
bool fillHalf(stb_vorbis *vorbis, int channels, DWORD offset, int &silentHalves)
{
    void *p1 = nullptr, *p2 = nullptr;
    DWORD b1 = 0, b2 = 0;
    {
        DsLock lock;
        if (FAILED(s_stream->Lock(offset, kStreamHalf, &p1, &b1, &p2, &b2, 0)))
            return false;
    }
    short *out = static_cast<short *>(p1);
    const int totalShorts = static_cast<int>(b1 / sizeof(short));
    int written = 0;
    if (vorbis != nullptr && silentHalves == 0)
    {
        while (written < totalShorts)
        {
            const int frames = stb_vorbis_get_samples_short_interleaved(vorbis, 2, out + written, totalShorts - written);
            if (frames <= 0)
                break;
            written += frames * 2;
        }
        (void)channels;
    }
    if (written < totalShorts)
    {
        std::memset(out + written, 0, static_cast<size_t>(totalShorts - written) * sizeof(short));
        ++silentHalves;
    }
    {
        DsLock lock;
        s_stream->Unlock(p1, b1, p2, b2);
    }
    return silentHalves < 3;
}

DWORD WINAPI streamThreadMain(LPVOID)
{
    int error = 0;
    stb_vorbis *vorbis = platformOpenVorbis(s_streamPath, &error);
    if (vorbis == nullptr)
    {
        s_streamKind = StreamKind::None;
        return 0;
    }
    const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    {
        DsLock lock;
        WAVEFORMATEX format;
        XAudioCreatePcmFormat(2, info.sample_rate, 16, &format);
        s_stream->Stop();
        s_stream->SetFormat(&format);
        s_stream->SetCurrentPosition(0);
        s_stream->SetVolume(toDsVolume(s_streamVolume));
    }
    int silentHalves = 0;
    fillHalf(vorbis, info.channels, 0, silentHalves);
    fillHalf(vorbis, info.channels, kStreamHalf, silentHalves);
    {
        DsLock lock;
        s_stream->Play(0, 0, DSBPLAY_LOOPING);
    }
    DWORD nextHalf = 0;   // the half to refill once the cursor leaves it
    bool running = true;
    while (running && s_streamStop == 0)
    {
        Sleep(20);
        DWORD play = 0, write = 0;
        {
            DsLock lock;
            s_stream->GetCurrentPosition(&play, &write);
        }
        const DWORD playingHalf = play < kStreamHalf ? 0 : kStreamHalf;
        if (playingHalf != nextHalf)
        {
            running = fillHalf(vorbis, info.channels, nextHalf, silentHalves);
            nextHalf = nextHalf == 0 ? kStreamHalf : 0;
        }
    }
    {
        DsLock lock;
        s_stream->Stop();
    }
    stb_vorbis_close(vorbis);
    s_streamKind = StreamKind::None;
    return 0;
}

void stopStream()
{
    if (s_streamThread != nullptr)
    {
        InterlockedExchange(&s_streamStop, 1);
        WaitForSingleObject(s_streamThread, INFINITE);
        CloseHandle(s_streamThread);
        s_streamThread = nullptr;
    }
    s_streamKind = StreamKind::None;
}

bool streamActive()
{
    if (s_streamThread != nullptr && s_streamKind == StreamKind::None)
        stopStream();   // finished on its own; reap the thread
    return s_streamKind != StreamKind::None;
}

bool startStream(const std::string &path, float volume, StreamKind kind)
{
    if (s_stream == nullptr)
        return false;
    stopStream();
    s_streamPath = path;
    s_streamVolume = volume;
    s_streamKind = kind;
    InterlockedExchange(&s_streamStop, 0);
    s_streamThread = CreateThread(nullptr, 64 * 1024, streamThreadMain, nullptr, 0, nullptr);
    if (s_streamThread == nullptr)
    {
        s_streamKind = StreamKind::None;
        return false;
    }
    return true;
}
// Releases every voice and DirectSound itself (sound off until re-created).
void releaseDirectSound()
{
    stopStream();
    clearSfx();
    DsLock lock;
    for (SfxVoice &voice : s_voices)
    {
        if (voice.buffer != nullptr)
            voice.buffer->Release();
        voice.buffer = nullptr;
    }
    if (s_stream != nullptr)
        s_stream->Release();
    s_stream = nullptr;
    if (s_ds != nullptr)
        s_ds->Release();
    s_ds = nullptr;
}
} // namespace

SoundManager::SoundManager()
    : soundPoolSounds(), soundPoolStreaming(), soundPoolMusic(), soundVolume(0),
      options(nullptr), rand(), ticksBeforeMusic(rand.nextInt(12000))
{
}

void SoundManager::tryToSetLibraryAndCodecs()
{
    if (loaded)
        return;
    if (!s_dsLockReady)
    {
        InitializeCriticalSection(&s_dsLock);
        s_dsLockReady = true;
    }
    // Stereo unless the player picked Dolby Digital in the options: then the
    // APU encodes a 5.1 mix to AC-3 on the optical/HDMI output. Must be set
    // before DirectSound is created.
    s_dolbyActive = options != nullptr && options->dolbyDigital;
    DirectSoundOverrideSpeakerConfig(s_dolbyActive
        ? DSSPEAKER_COMBINED(DSSPEAKER_SURROUND, DSSPEAKER_ENABLE_AC3)
        : DSSPEAKER_STEREO);
    if (FAILED(DirectSoundCreate(nullptr, &s_ds, nullptr)))
    {
        MC_LOG_ERROR("audio", "DirectSoundCreate failed; sound off\n");
        return;
    }

    WAVEFORMATEX format;
    XAudioCreatePcmFormat(1, 44100, 16, &format);
    DSBUFFERDESC desc;
    std::memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.lpwfxFormat = &format;
    desc.dwBufferBytes = 0;   // voices play cached PCM via SetBufferData
    for (SfxVoice &voice : s_voices)
        if (FAILED(s_ds->CreateSoundBuffer(&desc, &voice.buffer, nullptr)))
            voice.buffer = nullptr;

    XAudioCreatePcmFormat(2, 44100, 16, &format);
    desc.dwBufferBytes = kStreamBytes;
    if (FAILED(s_ds->CreateSoundBuffer(&desc, &s_stream, nullptr)))
        s_stream = nullptr;

    loaded = true;
    MC_LOG_INFO("audio", "DirectSound ready (%d voices, stream %s, %s)\n", kSfxVoices,
                s_stream ? "ok" : "none", s_dolbyActive ? "Dolby Digital" : "stereo");
}

void SoundManager::loadSoundSettings(GameSettings *gamesettings)
{
    soundPoolStreaming.motionX = false;
    options = gamesettings;
    if (!loaded && (gamesettings == nullptr || gamesettings->soundVolume != 0.0f || gamesettings->musicVolume != 0.0f))
        tryToSetLibraryAndCodecs();
}

void SoundManager::onSoundOptionsChanged()
{
    if (!loaded && options && (options->soundVolume != 0.0f || options->musicVolume != 0.0f))
        tryToSetLibraryAndCodecs();
    if (!loaded || options == nullptr)
        return;
    if (options->dolbyDigital != s_dolbyActive)
    {
        // Output mode changed: DirectSound only takes a speaker config at creation.
        releaseDirectSound();
        loaded = false;
        tryToSetLibraryAndCodecs();
        return;
    }
    if (s_streamKind == StreamKind::Music)
    {
        if (options->musicVolume <= 0.0f)
        {
            stopStream();
        }
        else if (s_stream != nullptr)
        {
            s_streamVolume = options->musicVolume;
            DsLock lock;
            s_stream->SetVolume(toDsVolume(s_streamVolume));
        }
    }
}

void SoundManager::closeMinecraft()
{
    if (loaded)
        releaseDirectSound();
    loaded = false;
}

void SoundManager::addSound(const jstring &s, const std::string &file)
{
    if (audioPathHasExtension(file, ".ogg"))
        soundPoolSounds.addSound(s, file);
}

void SoundManager::addStreaming(const jstring &s, const std::string &file)
{
    if (audioPathHasExtension(file, ".ogg"))
        soundPoolStreaming.addSound(s, file);
}

void SoundManager::addMusic(const jstring &s, const std::string &file)
{
    if (audioPathHasExtension(file, ".ogg"))
        soundPoolMusic.addSound(s, file);
}

bool SoundManager::playMusicFileNow(const std::string &file)
{
    if (!loaded || !options || options->musicVolume == 0.0f)
        return false;
    if (!audioPathHasExtension(file, ".ogg") || streamActive())
        return false;
    if (!startStream(file, options->musicVolume, StreamKind::Music))
        return false;
    ticksBeforeMusic = rand.nextInt(12000) + 12000;
    return true;
}

void SoundManager::playRandomMusicIfReady()
{
    if (!loaded || !options || options->musicVolume == 0.0f)
        return;
    if (streamActive())
        return;
    if (ticksBeforeMusic > 0)
    {
        ticksBeforeMusic--;
        return;
    }
    SoundPoolEntry *entry = soundPoolMusic.getRandomSound();
    ticksBeforeMusic = rand.nextInt(12000) + 12000;
    if (entry)
        startStream(entry->soundUrl, options->musicVolume, StreamKind::Music);
}

void SoundManager::setListenerPosition(EntityLiving *entityliving, float partialTick)
{
    if (!loaded)
        return;
    {
        // DirectSound's periodic housekeeping; the XDK wants it every frame.
        DsLock lock;
        DirectSoundDoWork();
    }
    if (options && options->soundVolume != 0.0f)
        updateAudioListener(s_listener, entityliving, partialTick);
}

void SoundManager::playStreaming(const jstring &s, float x, float y, float z, float f3, float)
{
    if (!loaded || !options || (options->soundVolume == 0.0f && !s.empty()))
        return;
    if (s.empty())
    {
        if (s_streamKind == StreamKind::Streaming)
            stopStream();
        return;
    }
    if (f3 <= 0.0f)
        return;
    const float attenuation = audioStreamingAttenuation(s_listener, x, y, z);
    if (attenuation <= 0.0f)
        return;
    SoundPoolEntry *entry = soundPoolStreaming.getRandomSoundFromSoundPool(s);
    if (entry && startStream(entry->soundUrl, 0.5f * attenuation * options->soundVolume, StreamKind::Streaming))
        ticksBeforeMusic = rand.nextInt(12000) + 12000;
}

void SoundManager::playSound(const jstring &s, float x, float y, float z, float volume, float pitch)
{
    if (!loaded || !options || options->soundVolume == 0.0f)
        return;
    SoundPoolEntry *entry = soundPoolSounds.getRandomSoundFromSoundPool(s);
    if (entry && volume > 0.0f)
    {
        const float attenuation = audioSpatialAttenuation(s_listener, x, y, z, volume);
        if (attenuation <= 0.0f)
            return;
        playSfx(entry->soundUrl, std::min(volume, 1.0f) * attenuation * options->soundVolume, pitch);
    }
}

void SoundManager::playSoundFX(const jstring &s, float volume, float pitch)
{
    if (!loaded || !options || options->soundVolume == 0.0f)
        return;
    SoundPoolEntry *entry = soundPoolSounds.getRandomSoundFromSoundPool(s);
    if (entry)
        playSfx(entry->soundUrl, std::min(volume, 1.0f) * 0.25f * options->soundVolume, pitch);
}

#endif
