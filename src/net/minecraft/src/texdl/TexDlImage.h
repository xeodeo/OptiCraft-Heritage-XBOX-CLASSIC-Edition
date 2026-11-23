#pragma once

#include <string>
#include <vector>

// Texture pack downloader: Modrinth project icons -> small RGBA squares.
namespace TexDl
{
// PNG / JPEG / GIF (first frame) through stb_image, WebP (lossy, lossless,
// alpha; not animated) through simplewebp. The picture is fitted, keeping
// its shape, into a size x size transparent square. Images bigger than
// 1024x1024 are refused before decoding (Modrinth's _96 icons are 96x96).
bool decodeIcon(const std::vector<unsigned char> &bytes, int size, std::vector<unsigned char> &rgbaOut,
    std::string &error);
}
