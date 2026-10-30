// SoundManager_PS3.cpp -- Minecraft sound on the PS3 (PSL1GHT libaudio).
//
// Same structure and policies as SoundManager_XBOX.cpp (bounded SFX cache,
// one streamed music voice, attenuation, music timer), but the PS3 has no
// hardware voices: a software mixer thread fills the 48 kHz stereo float
// audio port block by block.
//
//  * One-shot sounds are decoded once to 16-bit PCM into a bounded LRU cache
//    (kSfxCacheBudget). Voices play straight out of that memory, resampled
//    (source rate * pitch -> 48 kHz) with linear interpolation. An entry is
//    only evicted when no voice is playing it.
//  * Music and jukebox records stream through a ring of 16-bit stereo frames
//    that a decoder thread refills from stb_vorbis; the mixer resamples it.
//  * The mixer wakes on the audio port's notify event queue (20 ms timeout so
//    it always sees the stop flag) and keeps kAhead blocks queued ahead of the
//    hardware read index.
//
// Every voice / stream-state change is made under s_mixLock, which the mixer
// holds while it mixes one block (256 frames, a few microseconds of work).
#include "net/minecraft/src/SoundManager.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <audio/audio.h>
#include <sys/event_queue.h>
#include <sys/systime.h>
#include <sys/thread.h>

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
constexpr int kOutRate = 48000;
constexpr unsigned kBlocks = AUDIO_BLOCK_8;
constexpr unsigned kAhead = 4;              // blocks queued ahead of the read index (~21 ms)
constexpr int kMixerPriority = 400;         // above the game threads (1001)
constexpr int kStreamPriority = 900;
constexpr u64 kMixerStack = 64 * 1024;
constexpr u64 kStreamStack = 256 * 1024;    // stb_vorbis alloca()s its decode scratch

AudioListenerState s_listener;
std::mutex s_mixLock;

// ---- Audio port ------------------------------------------------------------
bool s_audioInit = false;
bool s_portOpen = false;
u32 s_port = 0;
audioPortConfig s_portConfig;
bool s_queueReady = false;
sys_event_queue_t s_queue = 0;
sys_ipc_key_t s_queueKey = 0;
bool s_portStarted = false;
bool s_mixerRunning = false;
sys_ppu_thread_t s_mixerThread = 0;
std::atomic<bool> s_mixerStop(false);

// ---- One-shot sounds -------------------------------------------------------
constexpr size_t kSfxCacheBudget = 2 * 1024 * 1024;   // decoded PCM kept resident
constexpr size_t kSfxMaxSample = 256 * 1024;          // longer clips are skipped
constexpr int kSfxVoices = 24;

struct SfxSample
{
    short *pcm = nullptr;
    size_t bytes = 0;
    uint32_t frames = 0;
    int channels = 1;
    int sampleRate = 44100;
    uint32_t lastUsed = 0;
};

struct SfxVoice
{
    const SfxSample *sample = nullptr;
    uint32_t pos = 0;        // source frame
    uint32_t frac = 0;       // 16.16 fraction of a source frame
    uint32_t step = 0;       // 16.16 source frames per output frame
    float volume = 0.0f;     // includes the 1/32768 PCM scale
    uint32_t startedAt = 0;
    bool active = false;
};

std::unordered_map<std::string, SfxSample> s_sfxCache;
std::unordered_set<std::string> s_sfxRejected;
size_t s_sfxCacheBytes = 0;
uint32_t s_sfxTick = 0;
SfxVoice s_voices[kSfxVoices];

bool sampleInUse(const SfxSample *sample)
{
    for (const SfxVoice &voice : s_voices)
        if (voice.active && voice.sample == sample)
            return true;
    return false;
}

bool evictOneSfx()
{
    std::lock_guard<std::mutex> lock(s_mixLock);
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
        if (voice.sample == &victim->second)
        {
            voice.active = false;
            voice.sample = nullptr;
        }
    s_sfxCacheBytes -= victim->second.bytes;
    std::free(victim->second.pcm);
    s_sfxCache.erase(victim);
    return true;
}

// Main thread only (the cache map itself is never touched by the mixer; it
// only follows SfxVoice::sample, and unordered_map nodes never move).
const SfxSample *getSfxSample(const std::string &path)
{
    auto it = s_sfxCache.find(path);
    if (it != s_sfxCache.end())
    {
        it->second.lastUsed = ++s_sfxTick;
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
    const size_t bytes = static_cast<size_t>(frames) * static_cast<size_t>(channels) * sizeof(short);
    if (bytes > kSfxMaxSample || channels < 1 || channels > 2 || rate <= 0)
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
    SfxSample &sample = s_sfxCache[path];
    sample.pcm = pcm;
    sample.bytes = bytes;
    sample.frames = static_cast<uint32_t>(frames);
    sample.channels = channels;
    sample.sampleRate = rate;
    sample.lastUsed = ++s_sfxTick;
    s_sfxCacheBytes += bytes;
    return &sample;
}

void playSfx(const std::string &path, float volume, float pitch)
{
    if (volume <= 0.0f)
        return;
    const SfxSample *sample = getSfxSample(path);
    if (sample == nullptr)
        return;

    double step = static_cast<double>(sample->sampleRate) * (pitch > 0.0f ? pitch : 1.0f) / kOutRate;
    step = std::max(0.05, std::min(8.0, step));

    std::lock_guard<std::mutex> lock(s_mixLock);
    // Free voice, else the one that started longest ago.
    SfxVoice *voice = nullptr;
    for (SfxVoice &candidate : s_voices)
    {
        if (!candidate.active) { voice = &candidate; break; }
        if (voice == nullptr || candidate.startedAt < voice->startedAt)
            voice = &candidate;
    }
    if (voice == nullptr)
        return;
    voice->sample = sample;
    voice->pos = 0;
    voice->frac = 0;
    voice->step = static_cast<uint32_t>(step * 65536.0);
    voice->volume = std::min(volume, 1.0f) * (1.0f / 32768.0f);
    voice->startedAt = s_sfxTick;
    voice->active = true;
}

void clearSfx()
{
    {
        std::lock_guard<std::mutex> lock(s_mixLock);
        for (SfxVoice &voice : s_voices)
        {
            voice.active = false;
            voice.sample = nullptr;
        }
    }
    for (auto &entry : s_sfxCache)
        std::free(entry.second.pcm);
    s_sfxCache.clear();
    s_sfxCacheBytes = 0;
}

// ---- Streaming (music, records) --------------------------------------------
enum class StreamKind { None, Music, Streaming };

constexpr uint32_t kRingFrames = 16384;   // stereo 16-bit, ~370 ms at 44.1 kHz
constexpr uint32_t kDecodeChunk = 2048;

short s_ring[kRingFrames * 2];
std::atomic<uint32_t> s_ringWrite(0);     // frames produced (decoder thread)
std::atomic<uint32_t> s_ringRead(0);      // frames consumed (mixer)
float s_ringFrac = 0.0f;                  // mixer only, under s_mixLock
double s_streamStep = 1.0;                // under s_mixLock
bool s_streamPlaying = false;             // under s_mixLock
std::atomic<float> s_streamVolume(1.0f);

bool s_streamThreadAlive = false;
sys_ppu_thread_t s_streamThread = 0;
std::atomic<bool> s_streamStop(false);
std::atomic<StreamKind> s_streamKind(StreamKind::None);
std::string s_streamPath;

void streamThreadMain(void *)
{
    int error = 0;
    stb_vorbis *vorbis = platformOpenVorbis(s_streamPath, &error);
    if (vorbis == nullptr)
    {
        MC_LOG_INFO("ps3.audio", "cannot open stream %s (%d)\n", s_streamPath.c_str(), error);
        s_streamKind = StreamKind::None;
        sysThreadExit(0);
        return;
    }
    const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    {
        std::lock_guard<std::mutex> lock(s_mixLock);
        s_ringWrite.store(0);
        s_ringRead.store(0);
        s_ringFrac = 0.0f;
        s_streamStep = static_cast<double>(info.sample_rate > 0 ? info.sample_rate : 44100) / kOutRate;
        s_streamPlaying = true;
    }

    bool eof = false;
    while (!s_streamStop.load())
    {
        const uint32_t write = s_ringWrite.load();
        const uint32_t queued = write - s_ringRead.load();
        if (eof)
        {
            if (queued <= 1)
                break;   // tail has played out
            sysUsleep(10000);
            continue;
        }
        const uint32_t space = kRingFrames - queued;
        if (space < kDecodeChunk / 2)
        {
            sysUsleep(10000);
            continue;
        }
        const uint32_t at = write % kRingFrames;
        const uint32_t chunk = std::min(std::min(space, kDecodeChunk), kRingFrames - at);
        const int got = stb_vorbis_get_samples_short_interleaved(vorbis, 2, s_ring + at * 2,
                                                                 static_cast<int>(chunk * 2));
        if (got <= 0)
            eof = true;
        else
            s_ringWrite.store(write + static_cast<uint32_t>(got));
    }
    {
        std::lock_guard<std::mutex> lock(s_mixLock);
        s_streamPlaying = false;
    }
    stb_vorbis_close(vorbis);
    s_streamKind = StreamKind::None;
    sysThreadExit(0);
}

void stopStream()
{
    if (s_streamThreadAlive)
    {
        s_streamStop = true;
        u64 ret = 0;
        sysThreadJoin(s_streamThread, &ret);
        s_streamThreadAlive = false;
    }
    {
        std::lock_guard<std::mutex> lock(s_mixLock);
        s_streamPlaying = false;
    }
    s_streamKind = StreamKind::None;
}

bool streamActive()
{
    if (s_streamThreadAlive && s_streamKind == StreamKind::None)
        stopStream();   // finished on its own; reap the thread
    return s_streamKind != StreamKind::None;
}

bool startStream(const std::string &path, float volume, StreamKind kind)
{
    if (!s_mixerRunning)
        return false;
    stopStream();
    s_streamPath = path;
    s_streamVolume = volume;
    s_streamKind = kind;
    s_streamStop = false;
    if (sysThreadCreate(&s_streamThread, streamThreadMain, nullptr, kStreamPriority, kStreamStack,
                        THREAD_JOINABLE, const_cast<char *>("audio stream")) != 0)
    {
        MC_LOG_INFO("ps3.audio", "stream thread create failed\n");
        s_streamKind = StreamKind::None;
        return false;
    }
    s_streamThreadAlive = true;
    return true;
}

// ---- Mixer -----------------------------------------------------------------
void mixVoice(SfxVoice &voice, float *mix)
{
    const SfxSample *sample = voice.sample;
    if (sample == nullptr)
    {
        voice.active = false;
        return;
    }
    const short *pcm = sample->pcm;
    const uint32_t frames = sample->frames;
    const float vol = voice.volume;
    uint32_t pos = voice.pos;
    uint32_t frac = voice.frac;
    const uint32_t step = voice.step;
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
    {
        if (pos >= frames)
        {
            voice.active = false;
            break;
        }
        const uint32_t next = pos + 1 < frames ? pos + 1 : pos;
        const float t = static_cast<float>(frac) * (1.0f / 65536.0f);
        if (sample->channels == 1)
        {
            const float a = pcm[pos], b = pcm[next];
            const float s = (a + (b - a) * t) * vol;
            mix[i * 2] += s;
            mix[i * 2 + 1] += s;
        }
        else
        {
            const float al = pcm[pos * 2], bl = pcm[next * 2];
            const float ar = pcm[pos * 2 + 1], br = pcm[next * 2 + 1];
            mix[i * 2] += (al + (bl - al) * t) * vol;
            mix[i * 2 + 1] += (ar + (br - ar) * t) * vol;
        }
        frac += step;
        pos += frac >> 16;
        frac &= 0xFFFF;
    }
    voice.pos = pos;
    voice.frac = frac;
}

void mixStream(float *mix)
{
    if (!s_streamPlaying)
        return;
    const float vol = s_streamVolume.load() * (1.0f / 32768.0f);
    const uint32_t write = s_ringWrite.load();
    uint32_t read = s_ringRead.load();
    float frac = s_ringFrac;
    const float step = static_cast<float>(s_streamStep);
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
    {
        if (write - read < 2)
            break;   // decoder behind (or tail): silence
        const short *a = s_ring + (read % kRingFrames) * 2;
        const short *b = s_ring + ((read + 1) % kRingFrames) * 2;
        mix[i * 2] += (a[0] + (b[0] - a[0]) * frac) * vol;
        mix[i * 2 + 1] += (a[1] + (b[1] - a[1]) * frac) * vol;
        frac += step;
        while (frac >= 1.0f)
        {
            frac -= 1.0f;
            ++read;
        }
    }
    s_ringFrac = frac;
    s_ringRead.store(read);
}

void fillBlock(unsigned index)
{
    float mix[AUDIO_BLOCK_SAMPLES * 2];
    std::memset(mix, 0, sizeof(mix));
    {
        std::lock_guard<std::mutex> lock(s_mixLock);
        for (SfxVoice &voice : s_voices)
            if (voice.active)
                mixVoice(voice, mix);
        mixStream(mix);
    }
    float *out = reinterpret_cast<float *>(static_cast<uintptr_t>(s_portConfig.audioDataStart)) +
                 static_cast<size_t>(index) * AUDIO_BLOCK_SAMPLES * 2;
    for (unsigned i = 0; i < AUDIO_BLOCK_SAMPLES * 2; ++i)
        out[i] = std::max(-1.0f, std::min(1.0f, mix[i]));
}

void mixerThreadMain(void *)
{
    const volatile u64 *readIndex =
        reinterpret_cast<const volatile u64 *>(static_cast<uintptr_t>(s_portConfig.readIndex));
    unsigned next = 1;   // block 0 is the one the hardware starts on
    while (!s_mixerStop.load())
    {
        sys_event_t event;
        sysEventQueueReceive(s_queue, &event, 20 * 1000);
        if (s_mixerStop.load())
            break;
        const unsigned current = static_cast<unsigned>(*readIndex % kBlocks);
        unsigned ahead = (next + kBlocks - current) % kBlocks;
        if (ahead == 0 || ahead > kAhead + 1)
        {
            next = (current + 1) % kBlocks;   // fell behind: restart right after the read block
            ahead = 1;
        }
        while (ahead <= kAhead)
        {
            fillBlock(next);
            next = (next + 1) % kBlocks;
            ++ahead;
        }
    }
    sysThreadExit(0);
}

// Releases every voice, the port and libaudio itself (sound off until re-created).
void shutdownAudio()
{
    stopStream();
    if (s_mixerRunning)
    {
        s_mixerStop = true;
        u64 ret = 0;
        sysThreadJoin(s_mixerThread, &ret);
        s_mixerRunning = false;
    }
    if (s_portStarted)
        audioPortStop(s_port);
    s_portStarted = false;
    if (s_queueReady)
        audioRemoveNotifyEventQueue(s_queueKey);
    if (s_portOpen)
        audioPortClose(s_port);
    s_portOpen = false;
    if (s_queueReady)
        sysEventQueueDestroy(s_queue, 0);
    s_queueReady = false;
    if (s_audioInit)
        audioQuit();
    s_audioInit = false;
    clearSfx();
}

bool startAudio()
{
    s32 ret = audioInit();
    if (ret != 0)
    {
        MC_LOG_INFO("ps3.audio", "audioInit failed %08x; sound off\n", static_cast<unsigned>(ret));
        return false;
    }
    s_audioInit = true;

    audioPortParam params;
    std::memset(&params, 0, sizeof(params));
    params.numChannels = AUDIO_PORT_2CH;
    params.numBlocks = kBlocks;
    params.attrib = AUDIO_PORT_INITLEVEL;
    params.level = 1.0f;
    ret = audioPortOpen(&params, &s_port);
    if (ret != 0)
    {
        MC_LOG_INFO("ps3.audio", "audioPortOpen failed %08x; sound off\n", static_cast<unsigned>(ret));
        shutdownAudio();
        return false;
    }
    s_portOpen = true;
    ret = audioGetPortConfig(s_port, &s_portConfig);
    if (ret != 0 || s_portConfig.audioDataStart == 0 || s_portConfig.readIndex == 0)
    {
        MC_LOG_INFO("ps3.audio", "audioGetPortConfig failed %08x; sound off\n", static_cast<unsigned>(ret));
        shutdownAudio();
        return false;
    }
    std::memset(reinterpret_cast<void *>(static_cast<uintptr_t>(s_portConfig.audioDataStart)), 0,
                static_cast<size_t>(kBlocks) * AUDIO_BLOCK_SAMPLES * 2 * sizeof(float));

    ret = audioCreateNotifyEventQueue(&s_queue, &s_queueKey);
    if (ret != 0)
    {
        MC_LOG_INFO("ps3.audio", "audioCreateNotifyEventQueue failed %08x; sound off\n", static_cast<unsigned>(ret));
        shutdownAudio();
        return false;
    }
    s_queueReady = true;
    audioSetNotifyEventQueue(s_queueKey);
    sysEventQueueDrain(s_queue);

    ret = audioPortStart(s_port);
    if (ret != 0)
    {
        MC_LOG_INFO("ps3.audio", "audioPortStart failed %08x; sound off\n", static_cast<unsigned>(ret));
        shutdownAudio();
        return false;
    }
    s_portStarted = true;

    s_mixerStop = false;
    if (sysThreadCreate(&s_mixerThread, mixerThreadMain, nullptr, kMixerPriority, kMixerStack,
                        THREAD_JOINABLE, const_cast<char *>("audio mixer")) != 0)
    {
        MC_LOG_INFO("ps3.audio", "mixer thread create failed; sound off\n");
        shutdownAudio();
        return false;
    }
    s_mixerRunning = true;
    MC_LOG_INFO("ps3.audio", "port %u open: %d Hz stereo, %u blocks of %d, %d voices\n",
                static_cast<unsigned>(s_port), kOutRate, static_cast<unsigned>(s_portConfig.numBlocks),
                AUDIO_BLOCK_SAMPLES, kSfxVoices);
    return true;
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
    if (!startAudio())
        return;
    loaded = true;
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
    if (s_streamKind == StreamKind::Music)
    {
        if (options->musicVolume <= 0.0f)
            stopStream();
        else
            s_streamVolume = options->musicVolume;
    }
}

void SoundManager::closeMinecraft()
{
    if (loaded)
    {
        shutdownAudio();
        MC_LOG_INFO("ps3.audio", "audio closed\n");
    }
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
