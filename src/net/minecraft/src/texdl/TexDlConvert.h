#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

// Texture pack downloader, step 2: turn a downloaded Java texture pack .zip
// into something the console draws with.
//
//   * A 1.2-1.4 pack (terrain.png + gui/items.png, up to 32x) on 1.2.5 is
//     kept as the .zip: TexturePackZip reads it directly.
//   * Everything else -- a 1.5 pack (one PNG per block in textures/blocks/),
//     a 1.6+ pack (assets/minecraft/...), a 64x atlas, or any old pack on
//     1.5.2 (cells the port reassigned need the game's textures) -- is
//     rebuilt here, the same way texturas/convertir_pack.py does it on the
//     PC: the atlases are assembled one texture at a time (at most 32 px per
//     block, bigger ones averaged down), the other files the game reads are
//     copied (shrunk when needed), and the result is written as one .pak
//     (scripts/make_pak.py format) next to the other packs. The raw zip is
//     then deleted.
//
// Blocking and slow (hundreds of small PNG decodes): run it on a worker
// thread. Peak memory: one atlas (1 MB at 32x) + the image being decoded.
namespace TexDl
{
enum class ConvertResult
{
    KeptZip,        // usable as it is: renamed to <name>.zip
    MadePak,        // <name>.pak written, zip deleted
    NotCompatible,  // no texture the console uses (modern 1.13+ pack, GUI/sound-only pack...)
    TooBig,         // atlas over 64x, or files too big to decode in 64 MB
    NoMemory,       // not enough free RAM right now
    WriteError,     // disk full / cannot write
    Broken,         // not a readable zip
    Cancelled
};

struct ConvertInput
{
    std::string zipPath;     // the downloaded file (any extension)
    std::string outDir;      // texture pack folder ("D:\\texturepacks")
    std::string packName;    // FATX-safe base name (no extension)
    // pack.txt when the pack has none (and no pack.mcmeta description).
    std::string fallbackLine1;
    std::string fallbackLine2;
    // The game's own terrain.png / gui/items.png (PNG bytes), for the cells a
    // 1.5 pack does not bring and the cells 1.5.2 reassigned.
    std::string defaultTerrainPng;
    std::string defaultItemsPng;
    bool game152 = false;
    int maxCell = 32;
    // Free RAM probe (KB), so the converter can refuse big decodes; null = unknown.
    std::function<long()> freeKb;
};

struct ConvertProgress
{
    std::atomic<int> done{0};
    std::atomic<int> total{0};
};

struct ConvertReport
{
    std::string outputPath;  // final .zip or .pak
    std::string format;      // "1.2-1.4", "1.5", "1.6+"
    int resolution = 0;      // pixels per block of the pack
    int cell = 0;            // pixels per block written
    int filesWritten = 0;
    int texturesPlaced = 0;  // 1.5 textures put into the atlases
    int skipped = 0;
    std::string detail;      // short reason when it failed (log / screen)
};

ConvertResult convertDownloadedPack(const ConvertInput &in, ConvertReport &report, ConvertProgress *progress,
    const std::atomic<bool> *cancel);

// Title -> file name FATX accepts (42 chars max, letters, digits, space and
// a few symbols), without extension; `maxLength` includes room for one.
std::string fatxSafeName(const std::string &title, std::size_t maxLength = 36);
}
