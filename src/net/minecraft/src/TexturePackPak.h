#pragma once

#include "TexturePackBase.h"

#include <memory>
#include <string>

class BufferedImage;
class Minecraft;
class PakArchive;

// A texture pack packed into one .pak file (scripts/make_pak.py run on the
// pack's folder: keys "terrain.png", "gui/items.png", "pack.txt", ...). One
// file to copy and one to open, instead of a hundred small ones on the
// console's disk. Whatever the pack lacks comes from the game's own data.
class TexturePackPak : public TexturePackBase
{
public:
    explicit TexturePackPak(const std::string &pakPath);
    ~TexturePackPak() override;

    bool isValid() const;
    void getTexturePackFolder(Minecraft *minecraft) override;
    void getResourceAsStream(Minecraft *minecraft) override;
    void bindThumbnailTexture(Minecraft *minecraft) override;
    std::istream *getResourceAsStream(const std::string &path) override;

private:
    std::string truncateDescription(const std::string &value) const;

    int_t thumbnailTexture;
    std::unique_ptr<BufferedImage> thumbnailImage;
    std::unique_ptr<PakArchive> pak;
};
