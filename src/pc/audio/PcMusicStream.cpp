#include "pc/audio/PcMusicStream.h"

#include "pc/external/stb_vorbis.h"
#include "platform/audio/AudioAssetFormat.h"
#include "platform/audio/VorbisAssetOpen.h"
#include "platform/storage/AssetPak.h"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <thread>

namespace
{
constexpr int kOutputSampleRate = 44100;
constexpr int kDecodeFrames = 4096;
constexpr std::size_t kRingSamples = static_cast<std::size_t>(kOutputSampleRate) * 2u;

std::array<float, kRingSamples> s_ring{};
std::size_t s_read = 0;
std::size_t s_write = 0;
std::size_t s_count = 0;
bool s_stop = false;
bool s_running = false;
bool s_eof = false;
std::thread s_thread;
std::mutex s_mutex;
std::condition_variable s_condition;

bool validateOgg(const std::string &path)
{
    int error = 0;
    stb_vorbis *vorbis = platformOpenVorbis(path, &error);
    if (vorbis == nullptr)
        return false;
    const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    stb_vorbis_close(vorbis);
    return info.channels >= 1 && info.channels <= 2 && info.sample_rate == kOutputSampleRate;
}

bool validatePcm(const std::string &path)
{
    if (AssetPak::isPakPath(path))
    {
        std::uint32_t offset = 0, size = 0;
        return AssetPak::locate(AssetPak::keyOf(path), &offset, &size) && size > 0 && (size & 1) == 0;
    }
    std::FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fclose(f);
    return size > 0 && (size & 1) == 0;
}

void decoderThread(std::string path)
{
    int error = 0;
    stb_vorbis *vorbis = platformOpenVorbis(path, &error);
    if (vorbis == nullptr)
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_eof = true;
        s_running = false;
        return;
    }

    const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    std::array<float, kDecodeFrames * 2> decoded{};
    std::array<float, kDecodeFrames * 2> stereo{};

    for (;;)
    {
        const int sourceChannels = info.channels;
        const int frames = stb_vorbis_get_samples_float_interleaved(
            vorbis, sourceChannels, decoded.data(), kDecodeFrames * sourceChannels);
        if (frames <= 0)
            break;

        for (int frame = 0; frame < frames; ++frame)
        {
            const float left = decoded[static_cast<std::size_t>(frame * sourceChannels)];
            const float right = sourceChannels > 1
                ? decoded[static_cast<std::size_t>(frame * sourceChannels + 1)]
                : left;
            stereo[static_cast<std::size_t>(frame * 2)] = left;
            stereo[static_cast<std::size_t>(frame * 2 + 1)] = right;
        }

        std::size_t source = 0;
        const std::size_t sampleCount = static_cast<std::size_t>(frames) * 2u;
        while (source < sampleCount)
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            s_condition.wait(lock, [] { return s_stop || s_count < kRingSamples; });
            if (s_stop)
            {
                stb_vorbis_close(vorbis);
                s_running = false;
                return;
            }

            const std::size_t freeSamples = kRingSamples - s_count;
            const std::size_t contiguous = std::min(freeSamples, kRingSamples - s_write);
            const std::size_t copyCount = std::min(contiguous, sampleCount - source);
            std::copy_n(stereo.data() + source, copyCount, s_ring.data() + s_write);
            s_write = (s_write + copyCount) % kRingSamples;
            s_count += copyCount;
            source += copyCount;
        }
    }

    stb_vorbis_close(vorbis);
    std::lock_guard<std::mutex> lock(s_mutex);
    s_eof = true;
    if (s_count == 0)
        s_running = false;
}

void decoderPcmThread(std::string path)
{
    std::FILE *file = nullptr;
    std::size_t remainingBytes = 0;

    if (AssetPak::isPakPath(path))
    {
        std::uint32_t dataOffset = 0;
        std::uint32_t size = 0;
        if (!AssetPak::locate(AssetPak::keyOf(path), &dataOffset, &size))
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_eof = true;
            s_running = false;
            return;
        }
        file = std::fopen(AssetPak::archivePath().c_str(), "rb");
        if (file == nullptr || std::fseek(file, static_cast<long>(dataOffset), SEEK_SET) != 0)
        {
            if (file) std::fclose(file);
            std::lock_guard<std::mutex> lock(s_mutex);
            s_eof = true;
            s_running = false;
            return;
        }
        remainingBytes = size;
    }
    else
    {
        file = std::fopen(path.c_str(), "rb");
        if (file == nullptr)
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_eof = true;
            s_running = false;
            return;
        }
        std::fseek(file, 0, SEEK_END);
        remainingBytes = static_cast<std::size_t>(std::ftell(file));
        std::fseek(file, 0, SEEK_SET);
    }

    constexpr std::size_t kChunkSamples = 2048;
    std::array<std::int16_t, kChunkSamples> rawSamples{};
    std::array<float, kChunkSamples * 4> stereo{};

    while (remainingBytes >= sizeof(std::int16_t))
    {
        const std::size_t toRead = std::min(remainingBytes / sizeof(std::int16_t), kChunkSamples);
        const std::size_t readCount = std::fread(rawSamples.data(), sizeof(std::int16_t), toRead, file);
        if (readCount == 0)
            break;

        remainingBytes -= readCount * sizeof(std::int16_t);

        const std::size_t inputFrames = readCount / 2;
        // Convert 22050 stereo to 44100 stereo
        for (std::size_t f = 0; f < inputFrames; ++f)
        {
            const float left = rawSamples[f * 2 + 0] / 32768.0f;
            const float right = rawSamples[f * 2 + 1] / 32768.0f;
            const std::size_t outBase = f * 4;
            stereo[outBase + 0] = left;
            stereo[outBase + 1] = right;
            stereo[outBase + 2] = left;
            stereo[outBase + 3] = right;
        }

        std::size_t source = 0;
        const std::size_t sampleCount = inputFrames * 4;
        while (source < sampleCount)
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            s_condition.wait(lock, [] { return s_stop || s_count < kRingSamples; });
            if (s_stop)
            {
                std::fclose(file);
                s_running = false;
                return;
            }

            const std::size_t freeSamples = kRingSamples - s_count;
            const std::size_t contiguous = std::min(freeSamples, kRingSamples - s_write);
            const std::size_t copyCount = std::min(contiguous, sampleCount - source);
            std::copy_n(stereo.data() + source, copyCount, s_ring.data() + s_write);
            s_write = (s_write + copyCount) % kRingSamples;
            s_count += copyCount;
            source += copyCount;
        }
    }

    std::fclose(file);
    std::lock_guard<std::mutex> lock(s_mutex);
    s_eof = true;
    if (s_count == 0)
        s_running = false;
}
}

namespace PcMusicStream
{
bool start(const std::string &path)
{
    const bool isPcm = audioPathHasExtension(path, ".pcm");
    const bool isOgg = audioPathHasExtension(path, ".ogg");

    if (isPcm)
    {
        if (!validatePcm(path))
            return false;
    }
    else if (isOgg)
    {
        if (!validateOgg(path))
            return false;
    }
    else
    {
        return false;
    }

    stop();
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_read = 0;
        s_write = 0;
        s_count = 0;
        s_stop = false;
        s_eof = false;
        s_running = true;
    }
    s_thread = isPcm ? std::thread(decoderPcmThread, path) : std::thread(decoderThread, path);
    {
        std::unique_lock<std::mutex> lock(s_mutex);
        s_condition.wait_for(lock, std::chrono::milliseconds(50), [] { return s_count >= 4096 || s_eof || s_stop; });
    }
    return true;
}

void stop()
{
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_stop = true;
    }
    s_condition.notify_all();
    if (s_thread.joinable())
        s_thread.join();
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_read = 0;
        s_write = 0;
        s_count = 0;
        s_stop = false;
        s_eof = false;
        s_running = false;
    }
}

bool active()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_running || s_count > 0;
}

void mix(float *stereoOutput, int frames, float volume)
{
    if (stereoOutput == nullptr || frames <= 0 || volume <= 0.0f)
        return;

    std::lock_guard<std::mutex> lock(s_mutex);
    const std::size_t requested = static_cast<std::size_t>(frames) * 2u;
    const std::size_t available = std::min(requested, s_count);
    for (std::size_t i = 0; i < available; ++i)
    {
        stereoOutput[i] += s_ring[s_read] * volume;
        s_read = (s_read + 1) % kRingSamples;
    }
    s_count -= available;
    if (s_eof && s_count == 0)
        s_running = false;
    s_condition.notify_one();
}
}
