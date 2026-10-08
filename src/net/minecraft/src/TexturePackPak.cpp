#include "TexturePackPak.h"

#include "Minecraft.h"
#include "RenderEngine.h"
#include "java/BufferedImage.h"
#include "java/String.h"
#include "platform/RenderAPI.h"
#include "platform/storage/PakArchive.h"

#include <sstream>
#include <string>

TexturePackPak::TexturePackPak(const std::string &pakPath)
    : thumbnailTexture(-1),
      pak(new PakArchive())
{
    if (!pak->open(pakPath))
        pak.reset();
    // The pack's name is its file name without the folders and ".pak".
    std::string name = pakPath;
    const std::size_t slash = name.find_last_of("\\/");
    if (slash != std::string::npos)
        name = name.substr(slash + 1);
    if (name.size() > 4)
        name = name.substr(0, name.size() - 4);
    texturePackFileName = name;
}

TexturePackPak::~TexturePackPak() = default;

bool TexturePackPak::isValid() const
{
    return pak != nullptr;
}

std::string TexturePackPak::truncateDescription(const std::string &value) const
{
    const jstring javaValue(value);
    return String::utf16Length(javaValue) > 34
        ? String::substringUtf16(javaValue, 0, 34)
        : javaValue;
}

void TexturePackPak::getTexturePackFolder(Minecraft *minecraft)
{
    (void)minecraft;
    firstDescriptionLine.clear();
    secondDescriptionLine.clear();
    thumbnailImage.reset();

    std::unique_ptr<std::istream> description(getResourceAsStream("/pack.txt"));
    if (description != nullptr && *description)
    {
        std::string line;
        if (std::getline(*description, line))
            firstDescriptionLine = truncateDescription(line);
        if (std::getline(*description, line))
            secondDescriptionLine = truncateDescription(line);
    }
    try
    {
        std::unique_ptr<std::istream> image(getResourceAsStream("/pack.png"));
        if (image != nullptr && *image)
            thumbnailImage.reset(new BufferedImage(BufferedImage::ImageIO_read(*image)));
    }
    catch (...)
    {
        thumbnailImage.reset();
    }
}

void TexturePackPak::getResourceAsStream(Minecraft *minecraft)
{
    if (minecraft != nullptr && minecraft->renderEngine != nullptr && thumbnailTexture >= 0)
    {
        minecraft->renderEngine->deleteTexture(thumbnailTexture);
        thumbnailTexture = -1;
    }
}

void TexturePackPak::bindThumbnailTexture(Minecraft *minecraft)
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

std::istream *TexturePackPak::getResourceAsStream(const std::string &path)
{
    if (pak != nullptr)
    {
        const std::string key = !path.empty() && path[0] == '/' ? path.substr(1) : path;
        if (const PakArchive::Entry *entry = pak->find(key))
        {
            std::string data(entry->size, '\0');
            if (entry->size == 0 || pak->read(*entry, 0, &data[0], entry->size))
                return new std::istringstream(std::move(data), std::ios::in | std::ios::binary);
        }
    }
    return TexturePackBase::getResourceAsStream(path);
}
