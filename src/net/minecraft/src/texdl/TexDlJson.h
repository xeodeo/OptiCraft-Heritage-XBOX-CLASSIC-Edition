#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Texture pack downloader: the few Modrinth API answers it needs, read with a
// tiny pull parser over the response bytes (no JSON library, no tree: only the
// fields kept below are copied, everything else is skipped in place).
namespace TexDl
{
struct SearchHit
{
    std::string projectId;
    std::string slug;
    std::string title;
    std::string author;
    std::string description;
    std::string iconUrl;
    std::int64_t downloads = 0;
    std::int64_t follows = 0;
};

struct SearchPage
{
    std::vector<SearchHit> hits;
    int totalHits = 0;
    int offset = 0;
};

struct VersionFile
{
    std::string url;
    std::string fileName;
    std::string sha1;        // lowercase hex, may be empty
    std::int64_t size = 0;
    bool primary = false;
};

struct ProjectVersion
{
    std::string name;
    std::string versionNumber;
    std::string datePublished;
    // The newest Minecraft version the file claims to support ("1.21.4"), and
    // whether it lists anything from 1.13 on (renamed textures: most likely
    // a modern-format pack the console cannot use).
    std::string newestGameVersion;
    bool listsModernVersions = false;
    int gameVersionCount = 0;
    VersionFile file;
};

// Search answer: {"hits":[...],"offset":n,"limit":n,"total_hits":n}.
bool parseSearch(const char *data, std::size_t size, SearchPage &out);

// /project/<id>/version answer (array). Picks the newest version that lists
// `gameVersion`, and its primary file (else its first .zip). False when the
// JSON is broken or nothing fits.
bool parseVersions(const char *data, std::size_t size, const std::string &gameVersion, ProjectVersion &out);

// pack.mcmeta (1.6+ packs): {"pack":{"description": "..." | {"text":...} | [...]}}
// -> plain text, Minecraft colour codes removed.
bool parseMcmetaDescription(const char *data, std::size_t size, std::string &out);

// Percent-encoding for a query parameter (RFC 3986 unreserved kept).
std::string urlEncode(const std::string &value);
}
