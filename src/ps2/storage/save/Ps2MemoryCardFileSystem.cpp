#ifdef PS2_PLATFORM

#include "platform/Log.h"
#include "ps2/storage/save/Ps2MemoryCardFileSystem.h"
#include "platform/storage/PathUtils.h"
#include "platform/storage/PosixFileSystem.h"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <limits>
#include <mutex>

#include <fcntl.h>
#include <libmc.h>
#include <unistd.h>

// See the matching comment in Ps2MemoryCard.cpp: mcOpen()'s mode argument goes
// to the IOP mc driver's own protocol, not to EE-side newlib's fcntl.h values,
// and the two disagree on O_RDONLY/O_WRONLY/O_RDWR (O_CREAT and O_TRUNC happen
// to match). A read-only mcOpen() built from newlib's O_RDONLY (0) requests no
// access at all, and mcRead() then denies it -- every readFile()/exists() call
// in this file goes through FIO_O_RDONLY, so this is why a world could be
// written and never read back.
#define MC_MODE_RDONLY 0x0001
#define MC_MODE_WRONLY 0x0002
#define MC_MODE_RDWR   0x0003
#define MC_MODE_CREAT  0x0200
#define MC_MODE_TRUNC  0x0400

#ifndef MC_O_CREAT_FILE
#define MC_O_CREAT_FILE (MC_MODE_CREAT | MC_MODE_RDWR | MC_MODE_TRUNC)
#endif
#ifndef FIO_O_RDONLY
#define FIO_O_RDONLY MC_MODE_RDONLY
#endif

namespace
{
    std::recursive_mutex s_mcIoMutex;
    unsigned char s_bounce[16384] __attribute__((aligned(64)));
    sceMcTblGetDir s_dirEntries[48] __attribute__((aligned(64)));

    int mcWait()
    {
        int command = 0;
        int result = -1;
        mcSync(0, &command, &result);
        return result;
    }

    std::string toCardPath(const std::string& path)
    {
        std::string result = PlatformStorage::normalizeSlashes(path);
        if (PlatformStorage::hasPrefix(result, "mc0:") || PlatformStorage::hasPrefix(result, "mc1:"))
            result.erase(0, 4);
        if (result.empty() || result.front() != '/')
            result.insert(result.begin(), '/');

        const std::size_t separator = result.find('/', 1);
        if (separator != std::string::npos)
            for (std::size_t i = separator + 1; i < result.size(); ++i)
                if (result[i] == '/')
                    result[i] = '_';
        return result;
    }

    bool mkdirCardPath(const std::string& cardPath)
    {
        std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
        for (std::size_t i = 1; i <= cardPath.size(); ++i)
        {
            if (i != cardPath.size() && cardPath[i] != '/')
                continue;

            const std::string directory = cardPath.substr(0, i);
            if (directory.empty() || directory == "/")
                continue;
            mcMkDir(0, 0, directory.c_str());
            mcWait();
        }
        return true;
    }

    bool readWithLibMc(const std::string& path, std::vector<unsigned char>& out)
    {
        std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
        const std::string cardPath = toCardPath(path);
        const int request = mcOpen(0, 0, cardPath.c_str(), FIO_O_RDONLY);
        if (request < 0)
            return false;
        const int fd = mcWait();
        if (fd < 0)
            return false;

        // Read sequentially from the initial file position. Extra seek operations are
        // unnecessary here and less reliable across MCSERV/XMCSERV variants
        // and emulators. mcRead advances the file pointer and returns 0 at EOF.
        const std::size_t readCap = 32u * 1024u * 1024u;
        out.reserve(4096);

        bool ok = true;
        for (;;)
        {
            const int requestRead = mcRead(fd, s_bounce, static_cast<int>(sizeof(s_bounce)));
            const int count = requestRead >= 0 ? mcWait() : -1;
            if (count < 0)
            {
                ok = false;
                break;
            }
            if (count == 0)
                break;

            const std::size_t remaining = readCap - out.size();
            if (static_cast<std::size_t>(count) > remaining)
            {
                ok = false;
                break;
            }

            out.insert(out.end(), s_bounce, s_bounce + count);
        }

        mcClose(fd);
        mcWait();
        if (!ok)
            out.clear();
        return ok && !out.empty();
    }

    bool readWithNewlib(const std::string& path, std::vector<unsigned char>& out)
    {
        FILE* input = std::fopen(path.c_str(), "rb");
        if (input == nullptr)
            return false;

        unsigned char buffer[4096];
        for (;;)
        {
            const std::size_t count = std::fread(buffer, 1, sizeof(buffer), input);
            if (count > 0)
                out.insert(out.end(), buffer, buffer + count);
            if (count < sizeof(buffer))
                break;
        }

        const bool ok = std::ferror(input) == 0;
        std::fclose(input);
        if (!ok)
            out.clear();
        return ok && !out.empty();
    }

    bool writeWithLibMc(const std::string& path, const void* data, std::size_t length, bool append)
    {
        std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
        const std::string cardPath = toCardPath(path);
        mkdirCardPath(PlatformStorage::parent(cardPath));

        if (!append)
        {
            mcDelete(0, 0, cardPath.c_str());
            mcWait();
        }

        const int flags = append ? (MC_MODE_CREAT | MC_MODE_RDWR) : MC_O_CREAT_FILE;
        const int request = mcOpen(0, 0, cardPath.c_str(), flags);
        if (request < 0)
            return false;
        const int fd = mcWait();
        if (fd < 0)
            return false;

        if (append)
        {
            mcSeek(fd, 0, SEEK_END);
            if (mcWait() < 0)
            {
                mcClose(fd);
                mcWait();
                return false;
            }
        }

        bool ok = true;
        const unsigned char* source = static_cast<const unsigned char*>(data);
        std::size_t remaining = length;
        while (remaining > 0)
        {
            const std::size_t count = remaining < sizeof(s_bounce) ? remaining : sizeof(s_bounce);
            std::memcpy(s_bounce, source, count);
            const int requestWrite = mcWrite(fd, s_bounce, static_cast<int>(count));
            const int result = requestWrite >= 0 ? mcWait() : -1;
            if (result != static_cast<int>(count))
            {
                ok = false;
                break;
            }
            source += count;
            remaining -= count;
        }

        mcFlush(fd);
        mcWait();
        mcClose(fd);
        mcWait();
        return ok;
    }
}

namespace Ps2MemoryCardFileSystem
{

bool handles(const std::string& path)
{
    return PlatformStorage::hasPrefix(path, "mc0:") || PlatformStorage::hasPrefix(path, "mc1:");
}

bool mkdirs(const std::string& path)
{
    return mkdirCardPath(toCardPath(path));
}

bool writeFile(const std::string& path, const void* data, std::size_t length)
{
    return writeWithLibMc(path, data, length, false);
}

bool appendFile(const std::string& path, const void* data, std::size_t length)
{
    return writeWithLibMc(path, data, length, true);
}

bool readFile(const std::string& path, std::vector<unsigned char>& out)
{
    out.clear();
    if (readWithLibMc(path, out))
    {
        MC_LOG_INFO("save", "[PS2] mc_read libmc '%s' bytes=%u\n", path.c_str(), static_cast<unsigned>(out.size()));
        return true;
    }

    out.clear();
    const bool ok = readWithNewlib(path, out);
    MC_LOG_INFO("save", "[PS2] mc_read newlib '%s' ok=%d bytes=%u\n",
                path.c_str(), static_cast<int>(ok), static_cast<unsigned>(out.size()));
    return ok;
}

std::recursive_mutex &getMcIoMutex()
{
    return s_mcIoMutex;
}

std::int64_t getFileSize(const std::string& path)
{
    std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
    const std::string cardPath = toCardPath(path);
    const int request = mcOpen(0, 0, cardPath.c_str(), FIO_O_RDONLY);
    if (request < 0)
        return -1;
    const int fd = mcWait();
    if (fd < 0)
        return -1;

    const int seekRequest = mcSeek(fd, 0, SEEK_END);
    const int size = seekRequest >= 0 ? mcWait() : -1;
    mcClose(fd);
    mcWait();
    return size >= 0 ? static_cast<std::int64_t>(size) : -1;
}

bool readFileRange(const std::string& path, std::size_t offset, void* out, std::size_t length)
{
    if ((out == nullptr && length != 0) ||
        offset > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    if (length == 0)
        return true;

    std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
    const std::string cardPath = toCardPath(path);
    const int request = mcOpen(0, 0, cardPath.c_str(), FIO_O_RDONLY);
    if (request < 0)
        return false;
    const int fd = mcWait();
    if (fd < 0)
        return false;

    const int seekRequest = mcSeek(fd, static_cast<int>(offset), SEEK_SET);
    if (seekRequest < 0 || mcWait() < 0)
    {
        mcClose(fd);
        mcWait();
        return false;
    }

    unsigned char* destination = static_cast<unsigned char*>(out);
    std::size_t remaining = length;
    bool ok = true;
    while (remaining > 0)
    {
        const std::size_t requestSize = remaining < sizeof(s_bounce) ? remaining : sizeof(s_bounce);
        const int requestRead = mcRead(fd, s_bounce, static_cast<int>(requestSize));
        const int count = requestRead >= 0 ? mcWait() : -1;
        if (count != static_cast<int>(requestSize))
        {
            ok = false;
            break;
        }

        std::memcpy(destination, s_bounce, requestSize);
        destination += requestSize;
        remaining -= requestSize;
    }

    mcClose(fd);
    mcWait();
    return ok;
}

bool exists(const std::string& path)
{
    std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
    const std::string cardPath = toCardPath(path);
    const int request = mcOpen(0, 0, cardPath.c_str(), FIO_O_RDONLY);
    if (request < 0)
        return false;
    const int fd = mcWait();
    if (fd < 0)
        return false;
    mcClose(fd);
    mcWait();
    return true;
}

bool removeFile(const std::string& path)
{
    std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
    const std::string cardPath = toCardPath(path);
    mcDelete(0, 0, cardPath.c_str());
    return mcWait() >= 0;
}

bool listEntries(const std::string& path, std::vector<std::string>& out)
{
    out.clear();

    std::lock_guard<std::recursive_mutex> lock(s_mcIoMutex);
    // Every other operation in this file goes through libmc (mcOpen/mcRead/
    // mcWrite/...) rather than newlib's POSIX opendir/readdir glue, for the
    // same reason the disc code stopped trusting that glue: it misreports on
    // this driver stack. Listing was the one place still assuming POSIX was
    // fine on mc0:. Use libmc's own directory API instead.
    std::string pattern = toCardPath(path);
    if (pattern.empty() || pattern.back() != '/')
        pattern.push_back('/');
    pattern.push_back('*');

    constexpr int kMaxEntriesPerCall = sizeof(s_dirEntries) / sizeof(s_dirEntries[0]);

    unsigned mode = 0;
    for (;;)
    {
        const int request = mcGetDir(0, 0, pattern.c_str(), mode, kMaxEntriesPerCall, s_dirEntries);
        if (request < 0)
            break;
        const int count = mcWait();
        if (count <= 0)
            break;

        for (int i = 0; i < count; ++i)
        {
            const char* name = reinterpret_cast<const char*>(s_dirEntries[i].EntryName);
            if (name[0] == '\0' || std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0)
                continue;
            out.emplace_back(name);
        }

        if (count < kMaxEntriesPerCall)
            break;
        // Continuation: subsequent mcGetDir calls resume where the last left off.
        mode = 1;
    }
    return true;
}

} // namespace Ps2MemoryCardFileSystem

#endif // PS2_PLATFORM
