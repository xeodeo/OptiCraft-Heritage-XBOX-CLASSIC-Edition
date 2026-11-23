#include "texdl/TexDlImage.h"

#if defined(XBOX_PLATFORM) || defined(TEXDL_HOST_TEST)

#include "stb_image.h"
#include "external/simplewebp.h"

#include <algorithm>
#include <cstring>

namespace TexDl
{
namespace
{
const int kMaxSide = 1024;

bool isWebp(const std::vector<unsigned char> &b)
{
    return b.size() >= 16 && std::memcmp(b.data(), "RIFF", 4) == 0 && std::memcmp(b.data() + 8, "WEBP", 4) == 0;
}

// Box-average (alpha weighted) into a size x size square, centred.
void fitSquare(const unsigned char *src, int w, int h, int size, std::vector<unsigned char> &out)
{
    out.assign(static_cast<std::size_t>(size) * size * 4, 0);
    const int scaleNum = std::max(w, h);
    const int dw = std::max(1, w * size / scaleNum);
    const int dh = std::max(1, h * size / scaleNum);
    const int ox = (size - dw) / 2;
    const int oy = (size - dh) / 2;
    for (int y = 0; y < dh; ++y)
    {
        const int sy0 = y * h / dh;
        const int sy1 = std::max(sy0 + 1, (y + 1) * h / dh);
        for (int x = 0; x < dw; ++x)
        {
            const int sx0 = x * w / dw;
            const int sx1 = std::max(sx0 + 1, (x + 1) * w / dw);
            unsigned long r = 0, g = 0, b = 0, a = 0, n = 0;
            for (int sy = sy0; sy < sy1; ++sy)
            {
                const unsigned char *p = src + (static_cast<std::size_t>(sy) * w + sx0) * 4;
                for (int sx = sx0; sx < sx1; ++sx, p += 4)
                {
                    r += static_cast<unsigned long>(p[0]) * p[3];
                    g += static_cast<unsigned long>(p[1]) * p[3];
                    b += static_cast<unsigned long>(p[2]) * p[3];
                    a += p[3];
                    ++n;
                }
            }
            unsigned char *o = &out[(static_cast<std::size_t>(oy + y) * size + ox + x) * 4];
            if (a > 0)
            {
                o[0] = static_cast<unsigned char>(r / a);
                o[1] = static_cast<unsigned char>(g / a);
                o[2] = static_cast<unsigned char>(b / a);
            }
            o[3] = static_cast<unsigned char>(a / n);
        }
    }
}
} // namespace

bool decodeIcon(const std::vector<unsigned char> &bytes, int size, std::vector<unsigned char> &rgbaOut,
    std::string &error)
{
    rgbaOut.clear();
    if (bytes.empty() || size <= 0)
    {
        error = "empty";
        return false;
    }
    if (isWebp(bytes))
    {
        simplewebp *webp = nullptr;
        // simplewebp only reads the buffer; the API is not const-correct.
        if (simplewebp_load_from_memory(const_cast<unsigned char *>(bytes.data()), bytes.size(), nullptr, &webp) !=
                SIMPLEWEBP_NO_ERROR ||
            webp == nullptr)
        {
            error = "webp: not readable";
            return false;
        }
        size_t w = 0, h = 0;
        simplewebp_get_dimensions(webp, &w, &h);
        if (w == 0 || h == 0 || w > static_cast<size_t>(kMaxSide) || h > static_cast<size_t>(kMaxSide))
        {
            simplewebp_unload(webp);
            error = "webp: too big";
            return false;
        }
        std::vector<unsigned char> rgba(w * h * 4);
        const simplewebp_error result = simplewebp_decode(webp, rgba.data(), nullptr);
        simplewebp_unload(webp);
        if (result != SIMPLEWEBP_NO_ERROR)
        {
            error = std::string("webp: ") + simplewebp_get_error_text(result);
            return false;
        }
        fitSquare(rgba.data(), static_cast<int>(w), static_cast<int>(h), size, rgbaOut);
        return true;
    }

    int w = 0, h = 0, comp = 0;
    if (!stbi_info_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &comp))
    {
        error = "unknown image format";
        return false;
    }
    if (w <= 0 || h <= 0 || w > kMaxSide || h > kMaxSide)
    {
        error = "image too big";
        return false;
    }
    stbi_uc *pixels = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()), &w, &h, &comp, 4);
    if (pixels == nullptr)
    {
        error = "decode failed";
        return false;
    }
    fitSquare(pixels, w, h, size, rgbaOut);
    stbi_image_free(pixels);
    return true;
}
} // namespace TexDl

#endif // XBOX_PLATFORM || TEXDL_HOST_TEST
