#include "texdl/TexDlConvert.h"

// Only the Xbox links minizip in this shape (and only it downloads packs);
// TEXDL_HOST_TEST builds it on the PC for the converter test.
#if defined(XBOX_PLATFORM) || defined(TEXDL_HOST_TEST)

#include "texdl/TexDlJson.h"
#include "texdl/TexDlTables.h"

#include "stb_image.h"
#include "unzip.h"
#include "zlib.h"

#if defined(XBOX_PLATFORM)
#include "platform/Storage.h"
#endif

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// PNG writer: stb_image_write with zlib's deflate (smaller files and less
// memory than stb's own compressor). Static: SkinManager has its own copy.
namespace
{
unsigned char *texdlZlibCompress(unsigned char *data, int length, int *outLength, int quality)
{
    (void)quality;
    uLongf bound = compressBound(static_cast<uLong>(length));
    unsigned char *out = static_cast<unsigned char *>(std::malloc(bound));
    if (out == nullptr)
        return nullptr;
    if (compress2(out, &bound, data, static_cast<uLong>(length), 6) != Z_OK)
    {
        std::free(out);
        return nullptr;
    }
    *outLength = static_cast<int>(bound);
    return out;
}
}
#define STBIW_ZLIB_COMPRESS texdlZlibCompress
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996 4244 4267)
#endif
#include "stb_image_write.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace TexDl
{
namespace
{
// ---------------------------------------------------------------- limits --
const std::size_t kMaxEntries = 20000;           // modern packs reach ~10k files
const std::uint32_t kMaxEntryBytes = 16u << 20;  // nothing a pack needs is bigger
const int kMaxAtlasSide = 1024;                  // 64x atlas: shrunk to 32x; more is refused
const std::size_t kMaxDecodeBytes = 16u << 20;   // RGBA of one image being decoded
// Leather colour baked into 1.5 icons for 1.2.5 (ItemArmor default, as the PC tool).
const int kLeather[3] = { 0xA0, 0x65, 0x40 };

// ----------------------------------------------------------------- image --
struct Img
{
    int w = 0;
    int h = 0;
    std::vector<unsigned char> px; // RGBA

    Img() = default;
    Img(int width, int height) : w(width), h(height), px(static_cast<std::size_t>(width) * height * 4, 0) {}
    bool empty() const { return w <= 0 || h <= 0; }
};

// Width/height from the PNG signature + IHDR (24 bytes); other formats
// through stb's header probe.
bool pngSize(const std::string &data, int &w, int &h)
{
    const unsigned char *b = reinterpret_cast<const unsigned char *>(data.data());
    if (data.size() >= 24 && b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G' && b[12] == 'I' &&
        b[13] == 'H' && b[14] == 'D' && b[15] == 'R')
    {
        const std::uint32_t pw = (std::uint32_t(b[16]) << 24) | (std::uint32_t(b[17]) << 16) | (std::uint32_t(b[18]) << 8) | b[19];
        const std::uint32_t ph = (std::uint32_t(b[20]) << 24) | (std::uint32_t(b[21]) << 16) | (std::uint32_t(b[22]) << 8) | b[23];
        if (pw == 0 || ph == 0 || pw > 0x8000u || ph > 0x8000u)
            return false;
        w = static_cast<int>(pw);
        h = static_cast<int>(ph);
        return true;
    }
    int comp = 0;
    return stbi_info_from_memory(b, static_cast<int>(data.size()), &w, &h, &comp) != 0;
}

bool decode(const std::string &data, Img &out, std::size_t maxBytes)
{
    int w = 0, h = 0;
    if (!pngSize(data, w, h) || w <= 0 || h <= 0)
        return false;
    if (static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u > maxBytes)
        return false;
    int comp = 0;
    stbi_uc *pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(data.data()),
        static_cast<int>(data.size()), &w, &h, &comp, 4);
    if (pixels == nullptr)
        return false;
    out.w = w;
    out.h = h;
    out.px.assign(pixels, pixels + static_cast<std::size_t>(w) * h * 4);
    stbi_image_free(pixels);
    return true;
}

bool encode(const Img &img, std::string &out)
{
    int length = 0;
    unsigned char *png = stbi_write_png_to_mem(img.px.data(), img.w * 4, img.w, img.h, 4, &length);
    if (png == nullptr)
        return false;
    out.assign(reinterpret_cast<const char *>(png), static_cast<std::size_t>(length));
    STBIW_FREE(png);
    return true;
}

Img crop(const Img &src, int x, int y, int w, int h)
{
    Img out(w, h);
    for (int row = 0; row < h; ++row)
    {
        const int sy = y + row;
        if (sy < 0 || sy >= src.h)
            continue;
        for (int col = 0; col < w; ++col)
        {
            const int sx = x + col;
            if (sx < 0 || sx >= src.w)
                continue;
            std::memcpy(&out.px[(static_cast<std::size_t>(row) * w + col) * 4],
                &src.px[(static_cast<std::size_t>(sy) * src.w + sx) * 4], 4);
        }
    }
    return out;
}

// Down: box average weighted by alpha (whole factors), nearest otherwise.
// Up: nearest (pixel art stays sharp).
Img fit(const Img &src, int w, int h)
{
    if (src.w == w && src.h == h)
        return src;
    Img out(w, h);
    if (w <= src.w && h <= src.h && src.w % w == 0 && src.h % h == 0)
    {
        const int fx = src.w / w;
        const int fy = src.h / h;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                unsigned long r = 0, g = 0, b = 0, a = 0;
                for (int yy = 0; yy < fy; ++yy)
                {
                    const unsigned char *p = &src.px[(static_cast<std::size_t>(y * fy + yy) * src.w + x * fx) * 4];
                    for (int xx = 0; xx < fx; ++xx, p += 4)
                    {
                        r += static_cast<unsigned long>(p[0]) * p[3];
                        g += static_cast<unsigned long>(p[1]) * p[3];
                        b += static_cast<unsigned long>(p[2]) * p[3];
                        a += p[3];
                    }
                }
                unsigned char *o = &out.px[(static_cast<std::size_t>(y) * w + x) * 4];
                const unsigned long n = static_cast<unsigned long>(fx) * fy;
                if (a > 0)
                {
                    o[0] = static_cast<unsigned char>(r / a);
                    o[1] = static_cast<unsigned char>(g / a);
                    o[2] = static_cast<unsigned char>(b / a);
                }
                o[3] = static_cast<unsigned char>(a / n);
            }
        return out;
    }
    for (int y = 0; y < h; ++y)
    {
        const int sy = static_cast<int>(static_cast<long long>(y) * src.h / h);
        for (int x = 0; x < w; ++x)
        {
            const int sx = static_cast<int>(static_cast<long long>(x) * src.w / w);
            std::memcpy(&out.px[(static_cast<std::size_t>(y) * w + x) * 4],
                &src.px[(static_cast<std::size_t>(sy) * src.w + sx) * 4], 4);
        }
    }
    return out;
}

void paste(Img &dst, const Img &src, int x, int y)
{
    for (int row = 0; row < src.h; ++row)
    {
        const int dy = y + row;
        if (dy < 0 || dy >= dst.h)
            continue;
        const int width = std::min(src.w, dst.w - x);
        if (width <= 0 || x < 0)
            continue;
        std::memcpy(&dst.px[(static_cast<std::size_t>(dy) * dst.w + x) * 4],
            &src.px[static_cast<std::size_t>(row) * src.w * 4], static_cast<std::size_t>(width) * 4);
    }
}

// 1.5 animations are vertical strips: keep the first square frame.
Img firstFrame(const Img &img)
{
    if (img.h > img.w && img.w > 0 && img.h % img.w == 0)
        return crop(img, 0, 0, img.w, img.w);
    return img;
}

void tintLeather(Img &img, const Img *overlay)
{
    for (std::size_t k = 0; k + 3 < img.px.size(); k += 4)
        for (int c = 0; c < 3; ++c)
            img.px[k + c] = static_cast<unsigned char>(img.px[k + c] * kLeather[c] / 255);
    if (overlay == nullptr || overlay->empty())
        return;
    const Img layer = fit(*overlay, img.w, img.h);
    for (std::size_t k = 0; k + 3 < img.px.size(); k += 4)
    {
        const int a = layer.px[k + 3];
        if (a == 0)
            continue;
        for (int c = 0; c < 3; ++c)
            img.px[k + c] = static_cast<unsigned char>((layer.px[k + c] * a + img.px[k + c] * (255 - a)) / 255);
        img.px[k + 3] = static_cast<unsigned char>(std::max<int>(img.px[k + 3], a));
    }
}

// ------------------------------------------------------------------- zip --
std::string lowerAscii(std::string value)
{
    for (char &c : value)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return value;
}

bool startsWith(const std::string &value, const char *prefix)
{
    const std::size_t n = std::strlen(prefix);
    return value.size() >= n && value.compare(0, n, prefix) == 0;
}

bool endsWith(const std::string &value, const char *suffix)
{
    const std::size_t n = std::strlen(suffix);
    return value.size() >= n && value.compare(value.size() - n, n, suffix) == 0;
}

struct ZipEntry
{
    std::string key; // lowercase, '/' separators
    std::uint32_t dirOffset;
    std::uint32_t fileIndex;
    std::uint32_t size;
};

class Zip
{
public:
    ~Zip()
    {
        if (zf != nullptr)
            unzClose(zf);
    }

    bool open(const std::string &path)
    {
        zlib_filefunc_def functions;
        fill_fopen_filefunc(&functions);
        zf = unzOpen2(path.c_str(), &functions);
        if (zf == nullptr)
            return false;
        char raw[512];
        for (int err = unzGoToFirstFile(zf); err == UNZ_OK; err = unzGoToNextFile(zf))
        {
            unz_file_info info;
            if (unzGetCurrentFileInfo(zf, &info, raw, sizeof(raw), nullptr, 0, nullptr, 0) != UNZ_OK)
                break;
            if (info.size_filename >= sizeof(raw))
                continue;
            std::string key;
            for (const char *c = raw; *c != '\0'; ++c)
            {
                char ch = *c == '\\' ? '/' : *c;
                if (ch == '/' && (key.empty() || key.back() == '/'))
                    continue;
                key.push_back(ch);
            }
            key = lowerAscii(key);
            if (key.empty() || key.back() == '/' || startsWith(key, "__macosx/") || endsWith(key, ".ds_store") ||
                key.find("../") != std::string::npos)
                continue;
            unz_file_pos position;
            if (unzGetFilePos(zf, &position) != UNZ_OK)
                continue;
            ZipEntry e;
            e.key = std::move(key);
            e.dirOffset = static_cast<std::uint32_t>(position.pos_in_zip_directory);
            e.fileIndex = static_cast<std::uint32_t>(position.num_of_file);
            e.size = static_cast<std::uint32_t>(info.uncompressed_size);
            entries.push_back(std::move(e));
            if (entries.size() >= kMaxEntries)
                break;
        }
        std::sort(entries.begin(), entries.end(), [](const ZipEntry &a, const ZipEntry &b) { return a.key < b.key; });
        return !entries.empty();
    }

    const ZipEntry *find(const std::string &key) const
    {
        auto it = std::lower_bound(entries.begin(), entries.end(), key,
            [](const ZipEntry &e, const std::string &v) { return e.key < v; });
        return it != entries.end() && it->key == key ? &*it : nullptr;
    }

    bool read(const ZipEntry &e, std::string &out, std::uint32_t limit = 0xffffffffu)
    {
        out.clear();
        if (e.size > kMaxEntryBytes)
            return false;
        const std::uint32_t wanted = std::min(limit, e.size);
        unz_file_pos position;
        position.pos_in_zip_directory = e.dirOffset;
        position.num_of_file = e.fileIndex;
        if (unzGoToFilePos(zf, &position) != UNZ_OK || unzOpenCurrentFile(zf) != UNZ_OK)
            return false;
        out.assign(wanted, '\0');
        const int got = wanted == 0 ? 0 : unzReadCurrentFile(zf, &out[0], wanted);
        const int closed = unzCloseCurrentFile(zf);
        if (got != static_cast<int>(wanted) || (wanted == e.size && closed != UNZ_OK))
        {
            std::string().swap(out);
            return false;
        }
        return true;
    }

    std::vector<ZipEntry> entries;

private:
    unzFile zf = nullptr;
};

// The pack inside the zip: its root folder, and for 1.6+ packs the
// assets/minecraft/ folder (pack.png / pack.mcmeta stay at the root).
struct PackView
{
    Zip &zip;
    std::string prefix;
    std::string rootPrefix;
    bool format16 = false;

    explicit PackView(Zip &z) : zip(z) {}

    const ZipEntry *find(const std::string &rel) const
    {
        if (format16 && (rel == "pack.png" || rel == "pack.mcmeta"))
            return zip.find(rootPrefix + rel);
        return zip.find(prefix + rel);
    }

    // Relative paths of every file under the prefix (and the 1.6 extras).
    std::vector<std::string> list() const
    {
        std::vector<std::string> out;
        for (const ZipEntry &e : zip.entries)
            if (e.key.size() > prefix.size() && e.key.compare(0, prefix.size(), prefix) == 0)
                out.push_back(e.key.substr(prefix.size()));
        if (format16)
        {
            if (zip.find(rootPrefix + "pack.png") != nullptr) out.push_back("pack.png");
            if (zip.find(rootPrefix + "pack.mcmeta") != nullptr) out.push_back("pack.mcmeta");
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
        return out;
    }
};

void findRoot(PackView &view)
{
    // Like convertir_pack.py: the shallowest folder holding a pack marker,
    // or the folder above textures/blocks/ or assets/minecraft/; on a tie the
    // one most markers point at.
    static const char *markers[] = { "pack.txt", "pack.mcmeta", "terrain.png", "pack.png" };
    std::vector<std::string> candidates;
    for (const ZipEntry &e : view.zip.entries)
    {
        for (const char *m : markers)
        {
            const std::size_t n = std::strlen(m);
            if (e.key == m || (e.key.size() > n && endsWith(e.key, m) && e.key[e.key.size() - n - 1] == '/'))
                candidates.push_back(e.key.substr(0, e.key.size() - n));
        }
        for (const char *m : { "textures/blocks/", "assets/minecraft/" })
        {
            const std::size_t i = e.key.find(m);
            if (i == 0 || (i != std::string::npos && e.key[i - 1] == '/'))
                candidates.push_back(e.key.substr(0, i));
        }
    }
    std::string best;
    if (!candidates.empty())
    {
        std::sort(candidates.begin(), candidates.end());
        long bestDepth = 0x7fffffff;
        long bestCount = 0;
        for (std::size_t i = 0; i < candidates.size();)
        {
            std::size_t j = i;
            while (j < candidates.size() && candidates[j] == candidates[i])
                ++j;
            const long depth = static_cast<long>(std::count(candidates[i].begin(), candidates[i].end(), '/'));
            const long count = static_cast<long>(j - i);
            if (depth < bestDepth || (depth == bestDepth && count > bestCount))
            {
                bestDepth = depth;
                bestCount = count;
                best = candidates[i];
            }
            i = j;
        }
    }
    view.prefix = best;
    view.rootPrefix = best;
    const std::string modern = best + "assets/minecraft/";
    for (const ZipEntry &e : view.zip.entries)
        if (startsWith(e.key, modern.c_str()))
        {
            view.prefix = modern;
            view.format16 = true;
            break;
        }
}

// ------------------------------------------------------------------- pak --
// scripts/make_pak.py format (PakArchive): header, data at 64-byte steps,
// then the table sorted by (FNV-1a of the lowercase key, key) and the names.
// The header is written last, once the table's place is known.
class PakWriter
{
public:
    ~PakWriter()
    {
        if (file != nullptr)
            std::fclose(file);
    }

    bool open(const std::string &path)
    {
        file = std::fopen(path.c_str(), "wb");
        if (file == nullptr)
            return false;
        unsigned char zero[64] = {};
        cursor = 64;
        return std::fwrite(zero, 1, 64, file) == 64;
    }

    bool has(const std::string &key) const
    {
        const std::string lower = lowerAscii(key);
        for (const Row &r : rows)
            if (r.lowerKey == lower)
                return true;
        return false;
    }

    bool add(const std::string &key, const std::string &data)
    {
        if (file == nullptr || has(key))
            return file != nullptr;
        Row row;
        row.lowerKey = lowerAscii(key);
        row.hash = fnv(row.lowerKey);
        row.nameOffset = static_cast<std::uint32_t>(names.size());
        row.dataOffset = cursor;
        row.size = static_cast<std::uint32_t>(data.size());
        names += key;
        names.push_back('\0');
        if (!data.empty() && std::fwrite(data.data(), 1, data.size(), file) != data.size())
            return false;
        cursor += row.size;
        if (!pad())
            return false;
        rows.push_back(std::move(row));
        return true;
    }

    bool finish()
    {
        if (file == nullptr)
            return false;
        std::sort(rows.begin(), rows.end(), [this](const Row &a, const Row &b) {
            if (a.hash != b.hash)
                return a.hash < b.hash;
            return std::strcmp(names.c_str() + a.nameOffset, names.c_str() + b.nameOffset) < 0;
        });
        const std::uint32_t tableOffset = cursor;
        const std::uint32_t namesOffset = tableOffset + static_cast<std::uint32_t>(rows.size()) * 16u;
        bool ok = true;
        for (const Row &r : rows)
        {
            unsigned char rec[16];
            be32(rec, r.hash);
            be32(rec + 4, r.nameOffset);
            be32(rec + 8, r.dataOffset);
            be32(rec + 12, r.size);
            ok = ok && std::fwrite(rec, 1, 16, file) == 16;
        }
        ok = ok && (names.empty() || std::fwrite(names.data(), 1, names.size(), file) == names.size());
        unsigned char header[32];
        std::memcpy(header, "MCPK", 4);
        be32(header + 4, 1);
        be32(header + 8, static_cast<std::uint32_t>(rows.size()));
        be32(header + 12, tableOffset);
        be32(header + 16, namesOffset);
        be32(header + 20, static_cast<std::uint32_t>(names.size()));
        be32(header + 24, 64);
        be32(header + 28, 0);
        ok = ok && std::fseek(file, 0, SEEK_SET) == 0 && std::fwrite(header, 1, 32, file) == 32;
        ok = (std::fclose(file) == 0) && ok;
        file = nullptr;
        return ok;
    }

    void abandon()
    {
        if (file != nullptr)
            std::fclose(file);
        file = nullptr;
    }

    std::size_t count() const { return rows.size(); }

private:
    struct Row
    {
        std::string lowerKey;
        std::uint32_t hash;
        std::uint32_t nameOffset;
        std::uint32_t dataOffset;
        std::uint32_t size;
    };

    static std::uint32_t fnv(const std::string &key)
    {
        std::uint32_t h = 0x811C9DC5u;
        for (unsigned char c : key)
            h = (h ^ c) * 0x01000193u;
        return h;
    }

    static void be32(unsigned char *p, std::uint32_t v)
    {
        p[0] = static_cast<unsigned char>(v >> 24);
        p[1] = static_cast<unsigned char>(v >> 16);
        p[2] = static_cast<unsigned char>(v >> 8);
        p[3] = static_cast<unsigned char>(v);
    }

    bool pad()
    {
        const std::uint32_t aligned = (cursor + 63u) & ~63u;
        if (aligned == cursor)
            return true;
        unsigned char zero[64] = {};
        const std::size_t n = aligned - cursor;
        cursor = aligned;
        return std::fwrite(zero, 1, n, file) == n;
    }

    std::FILE *file = nullptr;
    std::uint32_t cursor = 0;
    std::vector<Row> rows;
    std::string names;
};

// --------------------------------------------------------------- helpers --
const TexDlTables::RefSize *findRef(const std::string &rel, bool game152)
{
    if (game152)
    {
        for (const auto &r : TexDlTables::kRefSizes152)
            if (rel == r.path)
                return &r;
    }
    else
    {
        for (const auto &r : TexDlTables::kRefSizes125)
            if (rel == r.path)
                return &r;
    }
    return nullptr;
}

bool isNew152(int index, bool terrain)
{
    if (terrain)
    {
        for (std::uint16_t v : TexDlTables::kNew152Terrain)
            if (v == index)
                return true;
    }
    else
    {
        for (std::uint16_t v : TexDlTables::kNew152Items)
            if (v == index)
                return true;
    }
    return false;
}

bool isColourMap(const std::string &rel)
{
    return startsWith(rel, "misc/") && rel.find("color") != std::string::npos && endsWith(rel, ".png");
}

// Where a pack file goes in the output, or nullptr when it is left out
// (destino_de of convertir_pack.py, without the gui option).
bool keepFile(const std::string &rel, bool perFile)
{
    static const char *loose[] = { "terrain.png", "particles.png", "pack.png", "pack.txt", "ctm.png", "color.properties" };
    for (const char *l : loose)
        if (rel == l)
            return !(perFile && rel == "ctm.png");
    if (startsWith(rel, "custom_") && (endsWith(rel, ".png") || endsWith(rel, ".properties")))
        return !perFile;
    if (rel == "textures/items/clock.png" || rel == "textures/items/compass.png")
        return true;
    const std::size_t slash = rel.find('/');
    if (slash == std::string::npos)
        return false;
    const std::string top = rel.substr(0, slash);
    static const char *folders[] = { "mob", "item", "armor", "art", "misc", "environment", "terrain", "anim", "ctm" };
    for (const char *f : folders)
        if (top == f)
        {
            if (perFile && (top == "anim" || top == "ctm"))
                return false;
            return endsWith(rel, ".png") || (endsWith(rel, ".properties") && (top == "anim" || top == "ctm"));
        }
    return false;
}

std::string twoLines(const std::string &text)
{
    std::string out;
    int lines = 0;
    std::size_t start = 0;
    while (start <= text.size() && lines < 2)
    {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        std::string line = text.substr(start, end - start);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
            line.pop_back();
        std::size_t lead = 0;
        while (lead < line.size() && (line[lead] == ' ' || line[lead] == '\t'))
            ++lead;
        line.erase(0, lead);
        if (!line.empty())
        {
            out += line;
            out.push_back('\n');
            ++lines;
        }
        start = end + 1;
    }
    return out;
}

bool removeFile(const std::string &path)
{
#if defined(XBOX_PLATFORM)
    return PlatformStorage::removeFile(path);
#else
    return std::remove(path.c_str()) == 0;
#endif
}

// Replaces an existing destination.
bool renameFile(const std::string &from, const std::string &to)
{
#if defined(XBOX_PLATFORM)
    return PlatformStorage::renameFile(from, to);
#else
    std::remove(to.c_str());
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}

struct Context
{
    const ConvertInput &in;
    ConvertReport &report;
    ConvertProgress *progress;
    const std::atomic<bool> *cancel;
    std::size_t decodeLimit = kMaxDecodeBytes;

    bool cancelled() const { return cancel != nullptr && cancel->load(); }
    void step()
    {
        if (progress != nullptr)
            progress->done.fetch_add(1);
    }
};

// terrain.png (terrain=true) or gui/items.png, 16 x cell pixels a side.
// Returns false only on cancel; `img` stays empty when the pack has nothing
// for this atlas.
bool buildAtlas(Context &ctx, PackView &view, bool terrain, int cell, const std::vector<std::string> &files, Img &img,
    bool &tooBig)
{
    const char *atlasPath = terrain ? "terrain.png" : "gui/items.png";
    const std::string folder = terrain ? "textures/blocks/" : "textures/items/";
    const bool game152 = ctx.in.game152;
    const int side = 16 * cell;

    // Loose 1.5 textures in this folder (direct children only).
    std::vector<std::string> loose;
    for (const std::string &rel : files)
        if (startsWith(rel, folder.c_str()) && endsWith(rel, ".png") &&
            rel.find('/', folder.size()) == std::string::npos)
            loose.push_back(rel.substr(folder.size()));
    const ZipEntry *atlasEntry = view.find(atlasPath);
    if (atlasEntry == nullptr && loose.empty())
        return true;

    Img base;
    const std::string &defaultPng = terrain ? ctx.in.defaultTerrainPng : ctx.in.defaultItemsPng;
    std::string data;
    if (atlasEntry != nullptr && view.zip.read(*atlasEntry, data))
    {
        int w = 0, h = 0;
        if (pngSize(data, w, h) && (w > kMaxAtlasSide || h > kMaxAtlasSide))
        {
            tooBig = true;
            ctx.report.detail = std::string(atlasPath) + " " + std::to_string(w) + "x" + std::to_string(h);
            return true;
        }
        Img packAtlas;
        if (decode(data, packAtlas, ctx.decodeLimit))
            img = fit(packAtlas, side, side);
        else
        {
            tooBig = packAtlas.empty() && w * h * 4 > static_cast<int>(ctx.decodeLimit);
            if (tooBig)
                return true;
        }
    }
    std::string().swap(data);

    // The game's own atlas for what the pack lacks (and the 1.5.2 cells).
    Img defaults;
    if (!defaultPng.empty())
    {
        Img decoded;
        if (decode(defaultPng, decoded, ctx.decodeLimit))
            defaults = fit(decoded, side, side);
    }
    if (img.empty())
        img = defaults.empty() ? Img(side, side) : defaults;

    std::vector<int> placed;
    if (!loose.empty())
    {
        std::sort(loose.begin(), loose.end());
        auto has = [&](const std::string &name) {
            return std::binary_search(loose.begin(), loose.end(), name);
        };
        auto place = [&](const Img &icon, int index) {
            paste(img, fit(icon, cell, cell), index % 16 * cell, index / 16 * cell);
            placed.push_back(index);
            ++ctx.report.texturesPlaced;
        };
        const TexDlTables::Cell *cells = terrain ? TexDlTables::kBlockCells : TexDlTables::kItemCells;
        const std::size_t cellCount = terrain ? sizeof(TexDlTables::kBlockCells) / sizeof(TexDlTables::kBlockCells[0])
                                              : sizeof(TexDlTables::kItemCells) / sizeof(TexDlTables::kItemCells[0]);
        for (std::size_t i = 0; i < cellCount; ++i)
        {
            if (ctx.cancelled())
                return false;
            ctx.step();
            const std::string name = cells[i].name;
            const int index = cells[i].index;
            if (!game152 && isNew152(index, terrain))
                continue; // a cell 1.2.5 uses for something else
            if (!has(name))
                continue;
            const ZipEntry *e = view.find(folder + name);
            Img icon;
            if (e == nullptr || !view.zip.read(*e, data) || !decode(data, icon, ctx.decodeLimit))
            {
                ++ctx.report.skipped; // unreadable or too big: the cell keeps what it had
                continue;
            }
            icon = firstFrame(icon);
            const std::string base = name.substr(0, name.size() - 4);
            if (!terrain && !game152 &&
                (base == "helmetcloth" || base == "chestplatecloth" || base == "leggingscloth" || base == "bootscloth"))
            {
                // 1.5 leather is grey and tinted when drawn; 1.2.5 wants it brown.
                Img overlay;
                const ZipEntry *o = has(base + "_overlay.png") ? view.find(folder + base + "_overlay.png") : nullptr;
                std::string od;
                if (o != nullptr && view.zip.read(*o, od))
                    decode(od, overlay, ctx.decodeLimit);
                icon = fit(icon, cell, cell);
                tintLeather(icon, overlay.empty() ? nullptr : &overlay);
            }
            place(icon, index);
        }
        if (terrain)
        {
            // Flowing water / lava: one 2x2 texture in 1.5, four cells here.
            for (const auto &q : TexDlTables::kFlowQuadrants)
            {
                if (!has(q.file))
                    continue;
                const ZipEntry *e = view.find(folder + q.file);
                Img strip;
                if (e == nullptr || !view.zip.read(*e, data) || !decode(data, strip, ctx.decodeLimit))
                    continue;
                const Img frame = firstFrame(strip);
                const int half = frame.w / 2;
                if (half <= 0)
                    continue;
                place(crop(frame, q.qx * half, q.qy * half, half, half), q.index);
            }
        }
    }

    if (game152 && !defaults.empty())
    {
        // Cells 1.5.2 reassigned that the pack did not fill: the game's own.
        const std::uint16_t *list = terrain ? TexDlTables::kNew152Terrain : TexDlTables::kNew152Items;
        const std::size_t n = terrain ? sizeof(TexDlTables::kNew152Terrain) / sizeof(std::uint16_t)
                                      : sizeof(TexDlTables::kNew152Items) / sizeof(std::uint16_t);
        for (std::size_t i = 0; i < n; ++i)
        {
            const int index = list[i];
            if (std::find(placed.begin(), placed.end(), index) != placed.end())
                continue;
            const int x = index % 16 * cell;
            const int y = index / 16 * cell;
            paste(img, crop(defaults, x, y, cell, cell), x, y);
        }
    }
    return true;
}

int packResolution(PackView &view, const std::vector<std::string> &files)
{
    std::string data;
    int w = 0, h = 0;
    if (const ZipEntry *t = view.find("terrain.png"))
        if (view.zip.read(*t, data, 64) && pngSize(data, w, h) && w >= 16)
            return w / 16;
    static const char *probes[] = { "stone.png", "dirt.png", "planks.png", "wood.png", "cobblestone.png" };
    for (const char *p : probes)
        if (const ZipEntry *e = view.find(std::string("textures/blocks/") + p))
            if (view.zip.read(*e, data, 64) && pngSize(data, w, h) && w > 0)
                return w;
    std::vector<int> widths;
    for (const std::string &rel : files)
    {
        if (!startsWith(rel, "textures/blocks/") || !endsWith(rel, ".png"))
            continue;
        if (const ZipEntry *e = view.find(rel))
            if (view.zip.read(*e, data, 64) && pngSize(data, w, h) && w > 0)
                widths.push_back(w);
        if (widths.size() >= 20)
            break;
    }
    if (!widths.empty())
    {
        std::sort(widths.begin(), widths.end());
        return widths[widths.size() / 2];
    }
    return 16;
}
} // namespace

std::string fatxSafeName(const std::string &title, std::size_t maxLength)
{
    std::string out;
    for (std::size_t i = 0; i < title.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(title[i]);
        if (c == 0xC2 && i + 1 < title.size() && static_cast<unsigned char>(title[i + 1]) == 0xA7)
        {
            i += 2; // Minecraft colour code
            continue;
        }
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            std::strchr(" _-.()[]!#$%&'@^`{}~", static_cast<char>(c)) != nullptr;
        const char ch = ok && c != 0 ? static_cast<char>(c) : '_';
        if ((ch == '_' || ch == ' ') && !out.empty() && (out.back() == '_' || out.back() == ' '))
            continue;
        out.push_back(ch);
    }
    while (!out.empty() && (out.front() == ' ' || out.front() == '.' || out.front() == '_'))
        out.erase(out.begin());
    if (out.size() > maxLength)
        out.resize(maxLength);
    while (!out.empty() && (out.back() == ' ' || out.back() == '.' || out.back() == '_'))
        out.pop_back();
    return out.empty() ? std::string("pack") : out;
}

ConvertResult convertDownloadedPack(const ConvertInput &in, ConvertReport &report, ConvertProgress *progress,
    const std::atomic<bool> *cancel)
{
    report = ConvertReport();
    Context ctx{ in, report, progress, cancel };
    if (in.freeKb)
    {
        const long freeKb = in.freeKb();
        if (freeKb > 0)
        {
            // Leave room for the atlas, the encoder and the game: a third of
            // what is free for one decoded image, never more than 16 MB.
            const std::size_t third = static_cast<std::size_t>(freeKb) * 1024u / 3u;
            ctx.decodeLimit = std::min(kMaxDecodeBytes, third);
            if (freeKb < 6 * 1024)
            {
                report.detail = "free " + std::to_string(freeKb) + " KB";
                return ConvertResult::NoMemory;
            }
        }
    }

    Zip zip;
    if (!zip.open(in.zipPath))
    {
        report.detail = "unzOpen";
        return ConvertResult::Broken;
    }
    PackView view(zip);
    findRoot(view);
    const std::vector<std::string> files = view.list();

    bool hasLooseBlocks = false;
    bool hasLooseItems = false;
    for (const std::string &rel : files)
    {
        if (startsWith(rel, "textures/blocks/") && endsWith(rel, ".png")) hasLooseBlocks = true;
        if (startsWith(rel, "textures/items/") && endsWith(rel, ".png")) hasLooseItems = true;
    }
    const bool perFile = hasLooseBlocks;
    const bool hasAtlas = view.find("terrain.png") != nullptr;
    report.format = view.format16 ? "1.6+" : (perFile ? "1.5" : "1.2-1.4");
    report.resolution = packResolution(view, files);

    // Usable as it is? An old-style pack up to 32x on 1.2.5, with its atlas
    // where TexturePackZip looks for it (it roots the pack at terrain.png).
    if (!in.game152 && hasAtlas && !hasLooseBlocks && !hasLooseItems && !view.format16)
    {
        bool small = true;
        std::string head;
        int w = 0, h = 0;
        for (const char *atlas : { "terrain.png", "gui/items.png" })
            if (const ZipEntry *e = view.find(atlas))
                if (zip.read(*e, head, 64) && pngSize(head, w, h) && (w > 512 || h > 512))
                    small = false;
        if (small)
        {
            report.cell = report.resolution;
            report.outputPath = in.outDir + "\\" + in.packName + ".zip";
            // The rename happens in the caller once the zip is closed.
            return ConvertResult::KeptZip;
        }
    }

    // Pixels per block written: a power of two from 16 to maxCell.
    int cell = 16;
    while (cell * 2 <= std::min(report.resolution, in.maxCell))
        cell *= 2;
    report.cell = cell;

    // Progress: every atlas cell, plus every file looked at.
    if (progress != nullptr)
    {
        int total = static_cast<int>(files.size());
        if (hasLooseBlocks) total += static_cast<int>(sizeof(TexDlTables::kBlockCells) / sizeof(TexDlTables::kBlockCells[0]));
        if (hasLooseItems) total += static_cast<int>(sizeof(TexDlTables::kItemCells) / sizeof(TexDlTables::kItemCells[0]));
        progress->total.store(total);
        progress->done.store(0);
    }

    const std::string pakPath = in.outDir + "\\" + in.packName + ".pak";
    const std::string tmpPath = pakPath + ".tmp";
    PakWriter pak;
    if (!pak.open(tmpPath))
    {
        report.detail = "open " + tmpPath;
        return ConvertResult::WriteError;
    }
    auto failWith = [&](ConvertResult r) {
        pak.abandon();
        removeFile(tmpPath);
        return r;
    };

    int atlases = 0;
    for (int pass = 0; pass < 2; ++pass)
    {
        const bool terrain = pass == 0;
        Img atlas;
        bool tooBig = false;
        if (!buildAtlas(ctx, view, terrain, cell, files, atlas, tooBig))
            return failWith(ConvertResult::Cancelled);
        if (tooBig)
            return failWith(ConvertResult::TooBig);
        if (atlas.empty())
            continue;
        std::string png;
        if (!encode(atlas, png) || !pak.add(terrain ? "terrain.png" : "gui/items.png", png))
            return failWith(ConvertResult::WriteError);
        ++atlases;
    }

    int textures = 0;
    const int factor = std::max(1, report.resolution / cell);
    std::string data;
    std::string packText;
    for (const std::string &rel : files)
    {
        if (ctx.cancelled())
            return failWith(ConvertResult::Cancelled);
        ctx.step();
        if (rel == "terrain.png" || rel == "gui/items.png" || pak.has(rel))
            continue;
        if (rel == "pack.mcmeta")
        {
            const ZipEntry *e = view.find(rel);
            std::string description;
            if (e != nullptr && e->size < 64 * 1024 && zip.read(*e, data) &&
                parseMcmetaDescription(data.data(), data.size(), description))
                packText = twoLines(description);
            continue;
        }
        if (!keepFile(rel, perFile))
        {
            ++report.skipped;
            continue;
        }
        const ZipEntry *e = view.find(rel);
        if (e == nullptr || !zip.read(*e, data))
        {
            ++report.skipped;
            continue;
        }
        if (rel == "pack.txt")
        {
            const std::string lines = twoLines(data);
            if (!lines.empty())
                packText = lines;
            continue;
        }
        if (!endsWith(rel, ".png"))
        {
            if (data.size() > 64 * 1024 || !pak.add(rel, data))
                ++report.skipped;
            continue;
        }
        const bool optifine = startsWith(rel, "anim/") || startsWith(rel, "ctm/") || startsWith(rel, "custom_") ||
            rel == "ctm.png" || isColourMap(rel);
        const TexDlTables::RefSize *ref = findRef(rel, in.game152);
        if (ref == nullptr && rel != "pack.png" && !optifine)
        {
            ++report.skipped; // custom sky, random mobs, other versions' files...
            continue;
        }
        Img img;
        if (!decode(data, img, ctx.decodeLimit))
        {
            ++report.skipped;
            continue;
        }
        Img out = img;
        bool changed = false;
        if (rel == "pack.png")
        {
            if (img.w > 128 || img.h > 128)
            {
                const int k = std::max((img.w + 127) / 128, (img.h + 127) / 128);
                out = fit(img, std::max(1, img.w / k), std::max(1, img.h / k));
                changed = true;
            }
        }
        else if (isColourMap(rel) && ref != nullptr)
        {
            out = fit(img, ref->width, ref->height); // colour tables stay 256x256
            changed = out.w != img.w || out.h != img.h;
        }
        else if (ref != nullptr)
        {
            const int rw = ref->width;
            const int rh = ref->height;
            if (static_cast<long long>(img.h) * rw > static_cast<long long>(rh) * img.w && img.w >= rw)
            {
                // 1.5 made it taller (64x64 zombie): 1.2.5 models use the top.
                out = crop(img, 0, 0, img.w, img.w * rh / rw);
                changed = true;
            }
            const int maxW = rw * cell / 16;
            if (maxW > 0 && out.w > maxW)
            {
                out = fit(out, maxW, std::max(1, out.h * maxW / out.w));
                changed = true;
            }
        }
        else if (factor > 1 && img.w % factor == 0 && img.h % factor == 0)
        {
            out = fit(img, img.w / factor, img.h / factor);
            changed = true;
        }
        if (!in.game152 && perFile && (rel == "armor/cloth_1.png" || rel == "armor/cloth_2.png"))
        {
            Img overlay;
            const std::string overlayRel = rel.substr(0, rel.size() - 4) + "_b.png";
            std::string od;
            if (const ZipEntry *o = view.find(overlayRel))
                if (zip.read(*o, od))
                    decode(od, overlay, ctx.decodeLimit);
            tintLeather(out, overlay.empty() ? nullptr : &overlay);
            changed = true;
        }
        if (changed && !encode(out, data))
        {
            ++report.skipped;
            continue;
        }
        if (!pak.add(rel, data))
            return failWith(ConvertResult::WriteError);
        if (rel != "pack.png")
            ++textures;
    }
    std::string().swap(data);

    if (atlases == 0 && textures == 0)
    {
        report.detail = report.format + ": nothing the console draws";
        return failWith(ConvertResult::NotCompatible);
    }
    if (packText.empty())
    {
        packText = twoLines(in.fallbackLine1 + "\n" + in.fallbackLine2);
        if (packText.empty())
            packText = in.packName + "\n";
    }
    if (!pak.add("pack.txt", packText))
        return failWith(ConvertResult::WriteError);
    report.filesWritten = static_cast<int>(pak.count());
    if (!pak.finish())
    {
        removeFile(tmpPath);
        return ConvertResult::WriteError;
    }
    if (!renameFile(tmpPath, pakPath))
    {
        removeFile(tmpPath);
        report.detail = "rename " + pakPath;
        return ConvertResult::WriteError;
    }
    report.outputPath = pakPath;
    return ConvertResult::MadePak;
}
} // namespace TexDl

#endif // XBOX_PLATFORM || TEXDL_HOST_TEST
