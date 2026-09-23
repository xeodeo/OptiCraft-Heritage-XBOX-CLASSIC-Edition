// Xbox implementation of platform/storage/PosixFileSystem.h on top of XAPI.
//
// The consoles share PosixFileSystem.h as their low-level file API; PS2/Wii
// implement it with POSIX (PosixFileSystem.cpp). The Xbox has no POSIX layer,
// so this file provides the same functions with XAPI calls. Paths arrive with
// '/' separators (PathUtils normalizes to them); XAPI object paths need '\'.
#ifdef XBOX_PLATFORM

#include "xbox/XboxXtl.h"

#include "platform/storage/PathUtils.h"
#include "platform/storage/PosixFileSystem.h"

#include <cstdio>
#include <cstring>

namespace
{

std::string toXbox(const std::string& path)
{
    std::string out = path;
    for (char& c : out)
    {
        if (c == '/')
            c = '\\';
    }
    // "D:" alone is not a valid object path; the root is "D:\".
    if (out.size() == 2 && out[1] == ':')
        out += '\\';
    // FindFirstFile and friends reject a trailing separator (except the root).
    while (out.size() > 3 && out.back() == '\\')
        out.pop_back();
    return out;
}

// The 2003 XDK headers predate the INVALID_FILE_ATTRIBUTES macro.
const DWORD kInvalidFileAttributes = 0xFFFFFFFF;

bool attributes(const std::string& path, DWORD* out)
{
    const DWORD attrs = GetFileAttributesA(toXbox(path).c_str());
    if (attrs == kInvalidFileAttributes)
        return false;
    *out = attrs;
    return true;
}

} // namespace

namespace PlatformStorage
{

bool directoryAvailable(const std::string& path)
{
    return isDirectory(path);
}

bool fileReadable(const std::string& path)
{
    std::FILE* file = std::fopen(toXbox(path).c_str(), "rb");
    if (!file)
        return false;
    std::fclose(file);
    return true;
}

bool createFile(const std::string& path)
{
    HANDLE h = CreateFileA(toXbox(path).c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    CloseHandle(h);
    return true;
}

bool removePath(const std::string& path)
{
    const std::string native = toXbox(path);
    if (isDirectory(path))
        return RemoveDirectoryA(native.c_str()) != FALSE;
    return DeleteFileA(native.c_str()) != FALSE;
}

bool renamePath(const std::string& source, const std::string& destination)
{
    const std::string from = toXbox(source);
    const std::string to = toXbox(destination);
    // Java/POSIX rename replaces an existing destination; MoveFile does not.
    if (MoveFileA(from.c_str(), to.c_str()))
        return true;
    if (!isDirectory(destination) && DeleteFileA(to.c_str()))
        return MoveFileA(from.c_str(), to.c_str()) != FALSE;
    return false;
}

bool posixExists(const std::string& path)
{
    DWORD attrs = 0;
    return attributes(path, &attrs);
}

bool isDirectory(const std::string& path)
{
    DWORD attrs = 0;
    return attributes(path, &attrs) && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool isFile(const std::string& path)
{
    DWORD attrs = 0;
    return attributes(path, &attrs) && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::int64_t lastModifiedMs(const std::string& path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(toXbox(path).c_str(), GetFileExInfoStandard, &data))
        return 0;
    // FILETIME: 100 ns ticks since 1601-01-01; Java wants ms since 1970-01-01.
    const std::int64_t ticks = (static_cast<std::int64_t>(data.ftLastWriteTime.dwHighDateTime) << 32) |
                               data.ftLastWriteTime.dwLowDateTime;
    const std::int64_t epochDelta = 116444736000000000LL;
    return ticks > epochDelta ? (ticks - epochDelta) / 10000LL : 0;
}

std::int64_t fileSize(const std::string& path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(toXbox(path).c_str(), GetFileExInfoStandard, &data))
        return -1;
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return -1;
    return (static_cast<std::int64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
}

bool makeDirectory(const std::string& path, int)
{
    if (posixExists(path))
        return isDirectory(path);
    return CreateDirectoryA(toXbox(path).c_str(), NULL) != FALSE;
}

bool makeDirectories(const std::string& path, int)
{
    const std::string normalized = normalizeSlashes(path);
    for (std::size_t i = 1; i <= normalized.size(); ++i)
    {
        if (i != normalized.size() && normalized[i] != '/')
            continue;

        const std::string directory = normalized.substr(0, i);
        // Skip "D:" / "T:": drive roots always exist and cannot be created.
        if (directory.empty() || directory.back() == ':')
            continue;
        if (!CreateDirectoryA(toXbox(directory).c_str(), NULL) && !isDirectory(directory))
            return false;
    }
    return true;
}

bool listEntries(const std::string& path, std::vector<std::string>& out)
{
    out.clear();
    std::string pattern = toXbox(path);
    if (pattern.empty() || pattern.back() != '\\')
        pattern += '\\';
    pattern += "*";

    WIN32_FIND_DATAA data;
    HANDLE find = FindFirstFileA(pattern.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
        return isDirectory(path);  // an empty directory has no first entry

    do
    {
        if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
            continue;
        out.emplace_back(data.cFileName);
    } while (FindNextFileA(find, &data));

    FindClose(find);
    return true;
}

bool posixReadFile(const std::string& path, std::vector<unsigned char>& out)
{
    out.clear();
    std::FILE* input = std::fopen(toXbox(path).c_str(), "rb");
    if (input == nullptr)
        return false;

    unsigned char buffer[16384];
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
    return ok;
}

bool posixWriteFile(const std::string& path, const void* data, std::size_t length)
{
    if (!makeDirectories(parent(path)))
        return false;

    std::FILE* output = std::fopen(toXbox(path).c_str(), "wb");
    if (output == nullptr)
        return false;

    const bool wroteAll = length == 0 || std::fwrite(data, 1, length, output) == length;
    const bool flushed = std::fflush(output) == 0;
    const bool closed = std::fclose(output) == 0;
    return wroteAll && flushed && closed;
}

bool posixAppendFile(const std::string& path, const void* data, std::size_t length)
{
    const std::string parentDir = parent(path);
    if (!parentDir.empty() && !makeDirectories(parentDir))
        return false;

    std::FILE* output = std::fopen(toXbox(path).c_str(), "ab");
    if (output == nullptr)
        return false;

    const bool wroteAll = length == 0 || std::fwrite(data, 1, length, output) == length;
    const bool flushed = std::fflush(output) == 0;
    const bool closed = std::fclose(output) == 0;
    return wroteAll && flushed && closed;
}

} // namespace PlatformStorage

#endif // XBOX_PLATFORM
