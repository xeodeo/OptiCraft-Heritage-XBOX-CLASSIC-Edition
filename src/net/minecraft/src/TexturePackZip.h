#pragma once

#include "TexturePackBase.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class BufferedImage;
class Minecraft;

// A Java texture pack .zip read straight off the console's disk (Xbox only,
// minizip from external/zlib/contrib/minizip). The zip's central directory is
// walked once, when the pack is listed, into a sorted name -> position table
// (a few dozen bytes per file); a file is inflated only when the game asks
// for it, into one buffer the size of that file, like TexturePackPak. The zip
// itself is never held in memory. The file handle stays open only while the
// pack is the selected one (loadTexturePack / closeTexturePackFile).
//
// Both versions draw with the 1.2.5 atlases (terrain.png, gui/items.png), so
// only 1.2-1.4 style packs up to 32x work as they are. A 1.5-style zip (one
// file per block) or one whose atlas is bigger than 512 px (64x and up does
// not fit in 64 MB) is still listed, with the reason as its description, but
// cannot be selected: the PC converter (texturas/convertir_pack.py) makes a
// .pak out of those.
class TexturePackZip : public TexturePackBase
{
public:
    explicit TexturePackZip(const std::string &zipPath);
    ~TexturePackZip() override;

    // The zip opened and has at least one file.
    bool isValid() const;
    bool isUsable() const override;
    void getTexturePackFolder(Minecraft *minecraft) override;
    void getResourceAsStream(Minecraft *minecraft) override;
    void bindThumbnailTexture(Minecraft *minecraft) override;
    void loadTexturePack() override;
    void closeTexturePackFile() override;
    std::istream *getResourceAsStream(const std::string &path) override;
    std::vector<std::string> listResources(const std::string &prefix, const std::string &suffix) override;

private:
    enum class Status
    {
        Ok,
        Broken,
        Format15,
        TooLarge
    };

    struct Entry
    {
        std::string key;        // lowercase, '/' separators, pack root stripped
        std::uint32_t dirOffset; // unz_file_pos.pos_in_zip_directory
        std::uint32_t fileIndex; // unz_file_pos.num_of_file
        std::uint32_t size;      // uncompressed bytes
    };

    void buildIndex();
    const Entry *find(const std::string &path) const;
    // Reads up to `length` bytes from the start of the entry (all of it when
    // length >= size). Opens the zip for the call when it is not kept open.
    bool readEntry(const Entry &entry, std::uint32_t length, std::string &out);
    bool readPngSize(const Entry &entry, int &width, int &height);
    std::string truncateDescription(const std::string &value) const;

    std::string zipPath;
    void *zip; // unzFile while the pack is selected (or being indexed), else nullptr
    std::mutex zipMutex;
    std::vector<Entry> entries;
    Status status;
    int atlasSize;

    int_t thumbnailTexture;
    std::unique_ptr<BufferedImage> thumbnailImage;
};
