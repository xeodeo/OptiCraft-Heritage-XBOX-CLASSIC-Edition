#include "ps3/spu/Ps3SpuNoise.h"

#include "platform/PlatformConfig.h"
#include "platform/PlatformTuning.h"

#if PLATFORM_SPU_NOISE

#include <atomic>
#include <cstdint>
#include <cstring>
#include <type_traits>

#include "net/minecraft/src/NoiseGeneratorOctaves.h"
#include "net/minecraft/src/NoiseGeneratorPerlin.h"
#include "platform/Log.h"
#include "platform/PlatformCompat.h"
#include "ps3/spu/Ps3Spu.h"
#include "ps3/spu/tasks/PerlinNoise/PerlinNoiseJob.h"
#include "ps3/system/Ps3Stage.h"

static_assert(std::is_same<terrain_noise_real_t, double>::value, "SPU noise produces doubles");
static_assert(std::is_same<perlin_real_t, double>::value, "SPU noise mirrors the double Perlin path");
static_assert(sizeof(int_t) == sizeof(int), "permutation tables are copied as int");

#ifndef PS3_SPU_NOISE_VERIFY_CALLS
#define PS3_SPU_NOISE_VERIFY_CALLS 200
#endif

namespace
{
    constexpr int kMaxRequests = 8;

    PerlinNoise_DataIn s_in[kMaxRequests] __attribute__((aligned(128)));
    double s_out[kMaxRequests][PS3_SPU_NOISE_MAX_VALUES] __attribute__((aligned(128)));
    Ps3SpuJob s_jobs[kMaxRequests];

    std::atomic<bool> s_busy{false};
    bool s_disabled = false;

    int s_verifyRemaining = PS3_SPU_NOISE_VERIFY_CALLS;
    long s_verifyCalls = 0;
    long s_verifyMismatchCalls = 0;
    long s_verifyMismatchValues = 0;
    bool s_loggedFirstMismatch = false;
    bool s_loggedFirstUse = false;
    long s_verifyBatches = 0;
    long s_verifySpuMicros = 0;
    long s_verifyPpuMicros = 0;

    long s_statCalls = 0;
    long s_statBatches = 0;
    long s_statBatchMicros = 0;
    std::uint64_t s_statLastLogUs = 0;

    bool fill(const Ps3SpuNoiseRequest &request, PerlinNoise_DataIn &in, double *out)
    {
        const NoiseGeneratorOctaves &generator = *request.generator;
        const int_t levels = generator.octaveCount();
        if (levels <= 0 || levels > PerlinNoise_SPU::MAX_NOISE_LEVELS || generator.firstOctaveIndex() != 0)
            return false;
        if (request.sizeX <= 0 || request.sizeY <= 0 || request.sizeZ <= 0)
            return false;
        const long volume = static_cast<long>(request.sizeX) * request.sizeY * request.sizeZ;
        if (volume > PS3_SPU_NOISE_MAX_VALUES)
            return false;

        // 4J's PerlinNoise_DataIn::set / PerlinNoise_SPU::setup, from our classes.
        in.m_perlinData.levels = levels;
        for (int_t i = 0; i < levels; ++i)
        {
            const NoiseGeneratorPerlin *octave = generator.octave(i);
            if (octave == nullptr)
                return false;
            ImprovedNoise_SPU &level = in.m_perlinData.noiseLevels[i];
            std::memcpy(level.p, octave->permutationTable(), sizeof(level.p));
            level.scale = 1.0;
            level.xo = octave->xCoord;
            level.yo = octave->yCoord;
            level.zo = octave->zCoord;
        }
        in.m_outputBufferEA = static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(out));
        in.x = request.x;
        in.y = request.y;
        in.z = request.z;
        in.xSize = request.sizeX;
        in.ySize = request.sizeY;
        in.zSize = request.sizeZ;
        in.xScale = request.scaleX;
        in.yScale = request.scaleY;
        in.zScale = request.scaleZ;
        return true;
    }

    // The PPU reference for one request: exactly the call the generator
    // makes without SPUs (getBiomeGenForCoords is this with y = 10, sizeY = 1).
    void runOnPpu(const Ps3SpuNoiseRequest &request, TerrainNoiseBuffer &out)
    {
        request.generator->generateNoiseOctaves(out,
            static_cast<double>(request.x), static_cast<double>(request.y), static_cast<double>(request.z),
            request.sizeX, request.sizeY, request.sizeZ,
            request.scaleX, request.scaleY, request.scaleZ);
    }

    void verify(const Ps3SpuNoiseRequest &request, const double *spu)
    {
        TerrainNoiseBuffer &out = *request.out;
        runOnPpu(request, out);   // the terrain always gets the PPU values while verifying
        long mismatches = 0;
        for (std::size_t i = 0; i < out.size(); ++i)
        {
            std::uint64_t a, b;
            std::memcpy(&a, &out[i], sizeof(a));
            std::memcpy(&b, &spu[i], sizeof(b));
            if (a == b)
                continue;
            if (mismatches++ == 0 && !s_loggedFirstMismatch)
            {
                s_loggedFirstMismatch = true;
                MC_LOG_WARN("ps3.spu", "noise verify: first mismatch at call %ld value %u (%dx%dx%d, %d octaves): "
                            "PPU %016llx SPU %016llx\n",
                            s_verifyCalls, (unsigned)i, (int)request.sizeX, (int)request.sizeY, (int)request.sizeZ,
                            (int)request.generator->octaveCount(),
                            (unsigned long long)a, (unsigned long long)b);
            }
        }
        ++s_verifyCalls;
        if (mismatches > 0)
        {
            ++s_verifyMismatchCalls;
            s_verifyMismatchValues += mismatches;
        }
        if (s_verifyRemaining > 0)
            --s_verifyRemaining;
    }

    // After the batch that used up the verification: the verdict, and the
    // SPU batch time against the PPU doing the same fields (measured while
    // verifying), so one console run says both "equal?" and "faster?".
    void verifySummary()
    {
        MC_LOG_INFO("ps3.spu", "noise verify: %ld calls, %ld mismatches (%ld values); per batch: SPU %ld us, "
                    "PPU %ld us\n",
                    s_verifyCalls, s_verifyMismatchCalls, s_verifyMismatchValues,
                    s_verifyBatches > 0 ? s_verifySpuMicros / s_verifyBatches : 0L,
                    s_verifyBatches > 0 ? s_verifyPpuMicros / s_verifyBatches : 0L);
        if (s_verifyMismatchCalls > 0)
        {
            s_disabled = true;
            MC_LOG_WARN("ps3.spu", "noise: SPU result differs from the PPU; terrain noise stays on the PPU\n");
        }
        else
        {
            MC_LOG_INFO("ps3.spu", "noise: bit-for-bit equal; terrain noise runs on the SPUs only from now on\n");
        }
    }

    void logStats()
    {
        const std::uint64_t nowUs = PlatformCompat::getMonotonicMicros();
        if (s_statLastLogUs == 0)
        {
            s_statLastLogUs = nowUs;
            return;
        }
        if (nowUs - s_statLastLogUs < 5u * 1000u * 1000u)
            return;
        s_statLastLogUs = nowUs;
        const Ps3SpuStats spu = ps3SpuTakeStats();
        if (s_statBatches == 0 && spu.jobs == 0)
            return;
        MC_LOG_INFO("ps3.spu", "jobs=%ld failed=%ld spuUs=%ld ppuWaitUs=%ld noise: batches=%ld calls=%ld "
                    "batchUs=%ld (%ld us/batch)%s\n",
                    spu.jobs, spu.failed, spu.spuMicros, spu.waitMicros, s_statBatches, s_statCalls,
                    s_statBatchMicros, s_statBatches > 0 ? s_statBatchMicros / s_statBatches : 0L,
                    s_verifyRemaining > 0 ? " [verifying]" : "");
        s_statBatches = 0;
        s_statCalls = 0;
        s_statBatchMicros = 0;
    }

    struct BusyGuard
    {
        ~BusyGuard() { s_busy.store(false, std::memory_order_release); }
    };
}

bool ps3SpuNoiseGenerate(Ps3SpuNoiseRequest *requests, int count)
{
    if (s_disabled || count <= 0 || count > kMaxRequests)
        return false;
    // One batch at a time (the static buffers); another thread just uses the PPU.
    if (s_busy.exchange(true, std::memory_order_acq_rel))
        return false;
    BusyGuard guard;
    if (!ps3SpuAvailable())
        return false;

    PS3_STAGE_SCOPE("noiseSpu");
    const std::uint64_t t0 = PlatformCompat::getMonotonicMicros();

    for (int i = 0; i < count; ++i)
        if (!fill(requests[i], s_in[i], s_out[i]))
            return false;

    int submitted = 0;
    for (; submitted < count; ++submitted)
        if (!ps3SpuSubmit(s_jobs[submitted], PS3_SPU_JOB_PERLIN_NOISE, &s_in[submitted]))
            break;
    bool ok = submitted == count;
    for (int i = 0; i < submitted; ++i)
        if (!ps3SpuWait(s_jobs[i]))
            ok = false;
    if (!ok)
        return false;
    const std::uint64_t tSpu = PlatformCompat::getMonotonicMicros();

    if (!s_loggedFirstUse)
    {
        s_loggedFirstUse = true;
        MC_LOG_INFO("ps3.spu", "noise: first batch on the SPUs (%d fields, %ld us); verifying the first %d calls\n",
                    count, (long)(PlatformCompat::getMonotonicMicros() - t0), PS3_SPU_NOISE_VERIFY_CALLS);
    }

    // A batch that starts verifying is verified whole: if the summary
    // disables the SPU halfway, the rest still gets the PPU values.
    const bool verifyingBatch = s_verifyRemaining > 0;
    for (int i = 0; i < count; ++i)
    {
        const Ps3SpuNoiseRequest &request = requests[i];
        if (verifyingBatch)
        {
            verify(request, s_out[i]);
            continue;
        }
        TerrainNoiseBuffer &out = *request.out;
        out.resize(static_cast<std::size_t>(request.sizeX) * request.sizeY * request.sizeZ);
        std::memcpy(out.data(), s_out[i], out.size() * sizeof(double));
    }
    if (verifyingBatch)
    {
        ++s_verifyBatches;
        s_verifySpuMicros += static_cast<long>(tSpu - t0);
        s_verifyPpuMicros += static_cast<long>(PlatformCompat::getMonotonicMicros() - tSpu);
        if (s_verifyRemaining == 0)
            verifySummary();
    }

    ++s_statBatches;
    s_statCalls += count;
    s_statBatchMicros += static_cast<long>(PlatformCompat::getMonotonicMicros() - t0);
    logStats();
    return true;
}

#else

bool ps3SpuNoiseGenerate(Ps3SpuNoiseRequest *, int)
{
    return false;
}

#endif
