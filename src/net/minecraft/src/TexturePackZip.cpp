#include "TexturePackZip.h"

// Only the Xbox build compiles minizip in (cmake/xbox.cmake); the other
// platforms never list .zip packs through this class.
#if defined(XBOX_PLATFORM)

#include "Minecraft.h"
#include "RenderEngine.h"
#include "java/BufferedImage.h"
#include "java/String.h"
#include "platform/Log.h"
#include "platform/RenderAPI.h"
#include "unzip.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace
{
// 32x atlases are 512 px; 64x would be 4 MB each, twice over (GPU copy plus
// the CPU copy animated atlases keep), which 64 MB cannot spare.
const int kMaxAtlasPixels = 512;
// Thumbnails stay decoded in RAM for every listed pack.
const int kMaxThumbnailPixels = 256;
// No texture the game reads from a <=32x pack comes near this.
const std::uint32_t kMaxFileBytes = 4u * 1024u * 1024u;
// A 1.5 pack has ~1400 files; anything far past that is not a texture pack.
const std::size_t kMaxEntries = 8192;

std::string lowerAscii(std::string value)
{
    for (char &c : value)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return value;
}

// "\\Pack\\gui//items.png" -> "pack/gui/items.png", lowercase.
std::string normalizeKey(const std::string &path)
{
    std::string out;
    out.reserve(path.size());
    for (char c : path)
    {
        if (c == '\\')
            c = '/';
        if (c == '/' && (out.empty() || out.back() == '/'))
            continue;
        out.push_back(c);
    }
    while (out.compare(0, 2, "./") == 0)
        out.erase(0, 2);
    return lowerAscii(out);
}

bool endsWith(const std::string &value, const std::string &suffix)
{
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool isPackMarker(const std::string &key)
{
    const std::size_t slash = key.find_last_of('/');
    const std::string name = slash == std::string::npos ? key : key.substr(slash + 1);
    return name == "terrain.png" || name == "pack.txt";
}

unzFile openZip(const std::string &path)
{
    // The 32-bit stdio callbacks: the same FILE reads PakFile does on the
    // console, and no 64-bit seek functions the Xbox CRT may lack.
    zlib_filefunc_def functions;
    fill_fopen_filefunc(&functions);
    return unzOpen2(path.c_str(), &functions);
}
}

TexturePackZip::TexturePackZip(const std::string &zipPathIn)
    : zipPath(zipPathIn),
      zip(nullptr),
      status(Status::Broken),
      atlasSize(0),
      thumbnailTexture(-1)
{
    // The pack's name is its file name without the folders and ".zip".
    std::string name = zipPath;
    const std::size_t slash = name.find_last_of("\\/");
    if (slash != std::string::npos)
        name = name.substr(slash + 1);
    if (name.size() > 4)
        name = name.substr(0, name.size() - 4);
    texturePackFileName = name;

    buildIndex();
}

TexturePackZip::~TexturePackZip()
{
    closeTexturePackFile();
}

bool TexturePackZip::isValid() const
{
    return status != Status::Broken;
}

bool TexturePackZip::isUsable() const
{
    return status == Status::Ok;
}

void TexturePackZip::buildIndex()
{
    unzFile zf = openZip(zipPath);
    if (zf == nullptr)
    {
        MC_LOG_WARN("texturepack", "%s: not a readable zip, skipped\n", zipPath.c_str());
        return;
    }

    // One pass over the central directory. Directories, macOS metadata and
    // names too long for the buffer are left out.
    char rawName[256];
    for (int err = unzGoToFirstFile(zf); err == UNZ_OK; err = unzGoToNextFile(zf))
    {
        unz_file_info info;
        if (unzGetCurrentFileInfo(zf, &info, rawName, sizeof(rawName), nullptr, 0, nullptr, 0) != UNZ_OK)
            break;
        if (info.size_filename >= sizeof(rawName))
            continue;
        std::string key = normalizeKey(rawName);
        if (key.empty() || key.back() == '/' || key.compare(0, 9, "__macosx/") == 0 || endsWith(key, ".ds_store"))
            continue;
        unz_file_pos position;
        if (unzGetFilePos(zf, &position) != UNZ_OK)
            continue;
        Entry entry;
        entry.key = std::move(key);
        entry.dirOffset = static_cast<std::uint32_t>(position.pos_in_zip_directory);
        entry.fileIndex = static_cast<std::uint32_t>(position.num_of_file);
        entry.size = static_cast<std::uint32_t>(info.uncompressed_size);
        entries.push_back(std::move(entry));
        if (entries.size() >= kMaxEntries)
        {
            MC_LOG_WARN("texturepack", "%s: more than %u files, the rest are ignored\n",
                zipPath.c_str(), static_cast<unsigned>(kMaxEntries));
            break;
        }
    }

    // Many zips wrap the pack in one folder ("MyPack/terrain.png"). The pack's
    // root is the folder of the shallowest terrain.png or pack.txt; files
    // outside it (other packs, mod folders) are dropped.
    std::string root;
    std::size_t rootDepth = std::string::npos;
    for (const Entry &entry : entries)
    {
        if (!isPackMarker(entry.key))
            continue;
        const std::size_t depth = static_cast<std::size_t>(std::count(entry.key.begin(), entry.key.end(), '/'));
        if (depth < rootDepth)
        {
            rootDepth = depth;
            const std::size_t slash = entry.key.find_last_of('/');
            root = slash == std::string::npos ? std::string() : entry.key.substr(0, slash + 1);
        }
    }
    if (!root.empty())
    {
        std::vector<Entry> kept;
        kept.reserve(entries.size());
        for (Entry &entry : entries)
        {
            if (entry.key.compare(0, root.size(), root) != 0)
                continue;
            entry.key.erase(0, root.size());
            kept.push_back(std::move(entry));
        }
        entries.swap(kept);
    }
    std::sort(entries.begin(), entries.end(),
        [](const Entry &a, const Entry &b) { return a.key < b.key; });

    if (entries.empty())
    {
        unzClose(zf);
        MC_LOG_WARN("texturepack", "%s: empty zip, skipped\n", zipPath.c_str());
        return;
    }

    // Classify while the handle is still open (readEntry uses `zip`).
    zip = zf;
    status = Status::Ok;
    const Entry *terrain = find("/terrain.png");
    if (terrain == nullptr)
    {
        for (const Entry &entry : entries)
        {
            // 1.5 ("textures/blocks/stone.png") or 1.6 ("assets/minecraft/
            // textures/blocks/..."): one file per block, no atlas to draw with.
            if (entry.key.find("textures/blocks/") != std::string::npos)
            {
                status = Status::Format15;
                MC_LOG_WARN("texturepack", "%s: 1.5-style pack (textures/blocks, no terrain.png); convert it with the PC converter\n",
                    zipPath.c_str());
                break;
            }
        }
    }
    if (status == Status::Ok)
    {
        const char *atlases[] = { "/terrain.png", "/gui/items.png" };
        for (const char *atlas : atlases)
        {
            const Entry *entry = find(atlas);
            int width = 0;
            int height = 0;
            if (entry == nullptr || !readPngSize(*entry, width, height))
                continue;
            atlasSize = std::max(atlasSize, std::max(width, height) / 16);
            if (width > kMaxAtlasPixels || height > kMaxAtlasPixels)
            {
                status = Status::TooLarge;
                MC_LOG_WARN("texturepack", "%s: %s is %dx%d, bigger than 32x (512 px) does not fit in memory; convert it with the PC converter\n",
                    zipPath.c_str(), atlas, width, height);
                break;
            }
        }
    }

    zip = nullptr;
    unzClose(zf);

    if (status != Status::Ok)
    {
        // Listed only to say why: keep just what the list shows.
        std::vector<Entry> kept;
        for (Entry &entry : entries)
            if (entry.key == "pack.txt" || entry.key == "pack.png")
                kept.push_back(std::move(entry));
        entries.swap(kept);
    }
    else
    {
        MC_LOG_INFO("texturepack", "%s: %u files indexed%s\n", zipPath.c_str(),
            static_cast<unsigned>(entries.size()), root.empty() ? "" : " (inside a folder)");
    }
}

const TexturePackZip::Entry *TexturePackZip::find(const std::string &path) const
{
    std::string key = normalizeKey(path);
    while (!key.empty() && key.front() == '/')
        key.erase(key.begin());
    auto it = std::lower_bound(entries.begin(), entries.end(), key,
        [](const Entry &entry, const std::string &value) { return entry.key < value; });
    return it != entries.end() && it->key == key ? &*it : nullptr;
}

bool TexturePackZip::readEntry(const Entry &entry, std::uint32_t length, std::string &out)
{
    std::lock_guard<std::mutex> lock(zipMutex);
    unzFile zf = static_cast<unzFile>(zip);
    const bool temporary = zf == nullptr;
    if (temporary)
    {
        zf = openZip(zipPath);
        if (zf == nullptr)
            return false;
    }

    const std::uint32_t wanted = std::min(length, entry.size);
    bool ok = false;
    unz_file_pos position;
    position.pos_in_zip_directory = entry.dirOffset;
    position.num_of_file = entry.fileIndex;
    if (unzGoToFilePos(zf, &position) == UNZ_OK && unzOpenCurrentFile(zf) == UNZ_OK)
    {
        out.assign(wanted, '\0');
        const int got = wanted == 0 ? 0 : unzReadCurrentFile(zf, &out[0], wanted);
        ok = got == static_cast<int>(wanted);
        // Reports a CRC mismatch once the whole file has been read.
        const int closed = unzCloseCurrentFile(zf);
        if (wanted == entry.size && closed != UNZ_OK)
            ok = false;
    }
    if (temporary)
        unzClose(zf);
    if (!ok)
        std::string().swap(out);
    return ok;
}

bool TexturePackZip::readPngSize(const Entry &entry, int &width, int &height)
{
    // Signature (8), IHDR length (4) and tag (4), then big-endian width/height.
    std::string header;
    if (entry.size < 24 || !readEntry(entry, 24, header) || header.size() < 24)
        return false;
    const unsigned char *b = reinterpret_cast<const unsigned char *>(header.data());
    if (b[0] != 0x89 || b[1] != 'P' || b[2] != 'N' || b[3] != 'G' || b[12] != 'I' || b[13] != 'H' || b[14] != 'D' || b[15] != 'R')
        return false;
    const std::uint32_t w = (std::uint32_t(b[16]) << 24) | (std::uint32_t(b[17]) << 16) | (std::uint32_t(b[18]) << 8) | b[19];
    const std::uint32_t h = (std::uint32_t(b[20]) << 24) | (std::uint32_t(b[21]) << 16) | (std::uint32_t(b[22]) << 8) | b[23];
    width = w > 0x7fffffffu ? 0x7fffffff : static_cast<int>(w);
    height = h > 0x7fffffffu ? 0x7fffffff : static_cast<int>(h);
    return true;
}

std::string TexturePackZip::truncateDescription(const std::string &value) const
{
    const jstring javaValue(value);
    return String::utf16Length(javaValue) > 34
        ? String::substringUtf16(javaValue, 0, 34)
        : javaValue;
}

void TexturePackZip::getTexturePackFolder(Minecraft *minecraft)
{
    (void)minecraft;
    firstDescriptionLine.clear();
    secondDescriptionLine.clear();
    thumbnailImage.reset();

    // Read from the zip only: the game's own data has no pack.txt/pack.png
    // worth showing for someone else's pack.
    if (const Entry *text = find("/pack.txt"))
    {
        std::string data;
        if (readEntry(*text, 4096, data))
        {
            std::istringstream description(data);
            std::string line;
            if (std::getline(description, line))
                firstDescriptionLine = truncateDescription(line);
            if (std::getline(description, line))
                secondDescriptionLine = truncateDescription(line);
        }
    }
    if (const Entry *image = find("/pack.png"))
    {
        int width = 0;
        int height = 0;
        if (image->size <= kMaxFileBytes && readPngSize(*image, width, height)
            && width > 0 && height > 0 && width <= kMaxThumbnailPixels && height <= kMaxThumbnailPixels)
        {
            try
            {
                std::string data;
                if (readEntry(*image, image->size, data))
                {
                    std::istringstream stream(std::move(data), std::ios::in | std::ios::binary);
                    thumbnailImage.reset(new BufferedImage(BufferedImage::ImageIO_read(stream)));
                }
            }
            catch (...)
            {
                thumbnailImage.reset();
            }
        }
    }

    // The list's two grey lines say why the pack cannot be picked.
    if (status == Status::Format15)
    {
        firstDescriptionLine = "Formato 1.5: no se puede usar";
        secondDescriptionLine = "Convi\xC3\xA9rtelo con el conversor PC";
    }
    else if (status == Status::TooLarge)
    {
        firstDescriptionLine = "Pack de " + std::to_string(atlasSize) + "x: no cabe en memoria";
        secondDescriptionLine = "Convi\xC3\xA9rtelo con el conversor PC";
    }
}

void TexturePackZip::getResourceAsStream(Minecraft *minecraft)
{
    if (minecraft != nullptr && minecraft->renderEngine != nullptr && thumbnailTexture >= 0)
    {
        minecraft->renderEngine->deleteTexture(thumbnailTexture);
        thumbnailTexture = -1;
    }
}

void TexturePackZip::bindThumbnailTexture(Minecraft *minecraft)
{
    if (minecraft == nullptr || minecraft->renderEngine == nullptr)
        return;
    if (thumbnailImage != nullptr && thumbnailTexture < 0)
        thumbnailTexture = minecraft->renderEngine->allocateAndSetupTexture(thumbnailImage.get());
    if (thumbnailImage != nullptr && thumbnailTexture >= 0)
        minecraft->renderEngine->bindTexture(thumbnailTexture);
    else
        renderBindTexture(minecraft->renderEngine->getTexture("/gui/unknown_pack.png"));
}

void TexturePackZip::loadTexturePack()
{
    // Selected: keep one handle for the burst of texture loads that follows.
    std::lock_guard<std::mutex> lock(zipMutex);
    if (zip == nullptr && status == Status::Ok)
        zip = openZip(zipPath);
}

void TexturePackZip::closeTexturePackFile()
{
    std::lock_guard<std::mutex> lock(zipMutex);
    if (zip != nullptr)
    {
        unzClose(static_cast<unzFile>(zip));
        zip = nullptr;
    }
}

std::istream *TexturePackZip::getResourceAsStream(const std::string &path)
{
    if (status == Status::Ok)
    {
        if (const Entry *entry = find(path))
        {
            if (entry->size > kMaxFileBytes)
            {
                MC_LOG_WARN("texturepack", "%s: %s is %u bytes, skipped (game texture used)\n",
                    zipPath.c_str(), path.c_str(), static_cast<unsigned>(entry->size));
            }
            else
            {
                std::string data;
                if (readEntry(*entry, entry->size, data))
                    return new std::istringstream(std::move(data), std::ios::in | std::ios::binary);
                MC_LOG_WARN("texturepack", "%s: could not inflate %s\n", zipPath.c_str(), path.c_str());
            }
        }
    }
    return TexturePackBase::getResourceAsStream(path);
}

std::vector<std::string> TexturePackZip::listResources(const std::string &prefix, const std::string &suffix)
{
    std::vector<std::string> result;
    if (status != Status::Ok)
        return result;
    std::string wantPrefix = normalizeKey(prefix);
    while (!wantPrefix.empty() && wantPrefix.front() == '/')
        wantPrefix.erase(wantPrefix.begin());
    const std::string wantSuffix = lowerAscii(suffix);
    // Entries are sorted, so the prefix's files are one run.
    auto it = std::lower_bound(entries.begin(), entries.end(), wantPrefix,
        [](const Entry &entry, const std::string &value) { return entry.key < value; });
    for (; it != entries.end() && it->key.compare(0, wantPrefix.size(), wantPrefix) == 0; ++it)
        if (endsWith(it->key, wantSuffix))
            result.push_back('/' + it->key);
    return result;
}

#endif // XBOX_PLATFORM
