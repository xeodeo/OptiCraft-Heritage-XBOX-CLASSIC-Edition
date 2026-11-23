#include "texdl/GuiTexturePackDownload.h"

// Only the Xbox downloads (JavaNetworkStream, TLS, minizip in this shape).
#if defined(XBOX_PLATFORM)

#include "texdl/TexDlConvert.h"
#include "texdl/TexDlImage.h"
#include "texdl/TexDlJson.h"

#include "FontRenderer.h"
#include "GameResources.h"
#include "GameSettings.h"
#include "GuiTextField.h"
#include "Minecraft.h"
#include "RenderEngine.h"
#include "SoundManager.h"
#include "StringTranslate.h"
#include "TexturePackBase.h"
#include "TexturePackList.h"
#include "VirtualKeyboard.h"
#include "ControlIcon.h"
#include "java/BufferedImage.h"
#include "java/JavaNetworkStream.h"
#include "legacy/LegacyMenuWidgets.h"
#include "legacy/LegacyOptionText.h"
#include "legacy/LegacyUiTexture.h"
#include "pc/lwjgl/Keyboard.h"
#include "platform/Diagnostics.h"
#include "platform/Input.h"
#include "platform/Log.h"
#include "platform/PlatformConfig.h"
#include "platform/RenderAPI.h"
#include "platform/Storage.h"
#include "xbox/system/XboxTls.h"

#include "mbedtls/sha1.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <istream>
#include <iterator>
#include <map>
#include <mutex>
#include <thread>

// The 1.5.2 port has the compiled 1.5 cell table; 1.2.5 does not.
#if __has_include("Texture152Atlas.h")
#define TEXDL_GAME_152 1
#else
#define TEXDL_GAME_152 0
#endif

namespace
{
const char *const kGameVersion = TEXDL_GAME_152 ? "1.5.2" : "1.2.5";
const char *const kPackDir = "D:\\texturepacks";
const int kPageSize = 10;
const int kIconSize = 32;
const std::size_t kMaxIcons = 24;
const std::size_t kMaxJsonBytes = 512 * 1024;
const std::size_t kMaxIconBytes = 384 * 1024;
// A 32x pack is a few MB; 1.5 packs at 512x reach ~100 MB, still convertible
// texture by texture, but past this the download alone takes too long.
const long long kMaxPackBytes = 120LL * 1024 * 1024;
const char *const kSortIndex[4] = { "downloads", "newest", "updated", "follows" };

std::string trimSpaces(const std::string &value)
{
    std::size_t a = 0;
    std::size_t b = value.size();
    while (a < b && (value[a] == ' ' || value[a] == '\t'))
        ++a;
    while (b > a && (value[b - 1] == ' ' || value[b - 1] == '\t'))
        --b;
    return value.substr(a, b - a);
}

// Text from the web -> what the game font draws: Latin-1 range kept (as
// UTF-8), everything else becomes '?', control characters and colour codes
// dropped, runs of whitespace collapsed.
std::string cleanText(const std::string &in, std::size_t maxBytes = 400)
{
    std::string out;
    out.reserve(std::min(in.size(), maxBytes));
    for (std::size_t i = 0; i < in.size() && out.size() < maxBytes;)
    {
        const unsigned char c = static_cast<unsigned char>(in[i]);
        unsigned cp = c;
        std::size_t len = 1;
        if (c >= 0xC0 && c < 0xE0 && i + 1 < in.size()) { cp = ((c & 0x1Fu) << 6) | (in[i + 1] & 0x3F); len = 2; }
        else if (c >= 0xE0 && c < 0xF0 && i + 2 < in.size()) { cp = 0x10000; len = 3; }
        else if (c >= 0xF0 && i + 3 < in.size()) { cp = 0x10000; len = 4; }
        else if (c >= 0x80) { cp = '?'; }
        i += len;
        if (cp == 0xA7)
        {
            // Minecraft colour code: skip it and the next character.
            if (i < in.size())
                ++i;
            continue;
        }
        if (cp == '\n' || cp == '\r' || cp == '\t')
            cp = ' ';
        if (cp < 0x20 || cp == 0x7F)
            continue;
        if (cp == ' ' && (out.empty() || out.back() == ' '))
            continue;
        if (cp >= 0x100)
        {
            if (!out.empty() && out.back() == '?')
                continue;
            out.push_back('?');
        }
        else if (cp >= 0x80)
        {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
            out.push_back(static_cast<char>(cp));
    }
    return trimSpaces(out);
}

std::string formatCount(std::int64_t n, bool es)
{
    char buffer[32];
    const char sep = es ? ',' : '.';
    if (n >= 1000000)
        std::snprintf(buffer, sizeof(buffer), "%d%c%d M", static_cast<int>(n / 1000000), sep, static_cast<int>(n % 1000000 / 100000));
    else if (n >= 1000)
        std::snprintf(buffer, sizeof(buffer), "%d%c%d K", static_cast<int>(n / 1000), sep, static_cast<int>(n % 1000 / 100));
    else
        std::snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(n));
    return buffer;
}

std::string formatBytes(long long n, bool es)
{
    char buffer[32];
    const char sep = es ? ',' : '.';
    if (n >= 1024 * 1024)
        std::snprintf(buffer, sizeof(buffer), "%d%c%d MB", static_cast<int>(n / (1024 * 1024)), sep,
            static_cast<int>(n % (1024 * 1024) * 10 / (1024 * 1024)));
    else
        std::snprintf(buffer, sizeof(buffer), "%d KB", static_cast<int>((n + 1023) / 1024));
    return buffer;
}

std::string toHex(const unsigned char *data, std::size_t n)
{
    static const char digits[] = "0123456789abcdef";
    std::string out;
    for (std::size_t i = 0; i < n; ++i)
    {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 15]);
    }
    return out;
}

JavaNetwork::HttpStreamOptions apiOptions(const std::atomic<bool> *cancel)
{
    JavaNetwork::HttpStreamOptions o;
    o.connectTimeoutMs = 15000;
    o.idleTimeoutMs = 20000;
    o.cancel = cancel;
    o.accept = "application/json";
    return o;
}

std::string readGameResource(const char *path)
{
    std::unique_ptr<std::istream> in = GameResources::open(path);
    if (in == nullptr)
        return std::string();
    return std::string(std::istreambuf_iterator<char>(*in), std::istreambuf_iterator<char>());
}
} // namespace

// ------------------------------------------------------------------ jobs --
struct GuiTexturePackDownload::Hit
{
    TexDl::SearchHit raw;
    std::string title;
    std::string author;
    std::string description;
};

struct GuiTexturePackDownload::PageJob
{
    struct Icon
    {
        std::string projectId;
        std::vector<unsigned char> rgba;
        bool ready = false;
        bool taken = false;
    };

    std::string url;
    std::vector<std::string> cachedIcons; // already on the main thread: not fetched
    std::atomic<bool> cancel{false};
    std::atomic<bool> pageDone{false};
    bool pageTaken = false;
    std::mutex mtx;
    JavaNetwork::HttpResult result = JavaNetwork::HttpResult::NoConnection;
    bool parsed = false;
    bool fetchAll = false;
    // Tab 0: packs that list newer versions too are checked one by one (their
    // file for this version may be a real old-format one, like Gerudoku Faithful).
    std::vector<TexDl::SearchHit> verified;
    int verifyDone = 0;
    int verifyTotal = 0;
    TexDl::SearchPage page;
    std::vector<Icon> iconsOut;
};

struct GuiTexturePackDownload::VersionJob
{
    std::string url;
    std::atomic<bool> cancel{false};
    std::atomic<bool> done{false};
    JavaNetwork::HttpResult result = JavaNetwork::HttpResult::NoConnection;
    bool found = false;
    TexDl::ProjectVersion version;
};

struct GuiTexturePackDownload::DownloadJob
{
    enum Phase { Connecting = 0, Downloading, Converting, Done };
    enum Outcome
    {
        None, Installed, NoConnection, NotFound, TooLarge, WriteError, BadFile, NotCompatible, TooBig, NoMemory, Cancelled
    };

    // In
    std::string url;
    std::string sha1;
    long long expectedSize = 0;
    std::string packName;
    std::string title;
    std::string author;
    std::string defaultTerrain;
    std::string defaultItems;

    // Progress (read by the render thread)
    std::atomic<bool> cancel{false};
    std::atomic<int> phase{Connecting};
    std::atomic<int> received{0};
    std::atomic<int> total{0};
    TexDl::ConvertProgress convert;

    // Out (valid once phase == Done)
    Outcome outcome = None;
    bool converted = false;
    TexDl::ConvertReport report;
    std::string detail;
};

namespace
{
// Body -> <pack>.dl on disk, hashing as it goes.
class FileSink : public JavaNetwork::HttpStreamSink
{
public:
    FileSink(std::FILE *f, GuiTexturePackDownload::DownloadJob &j) : file(f), job(j)
    {
        mbedtls_sha1_init(&sha);
        mbedtls_sha1_starts(&sha);
    }
    ~FileSink() override { mbedtls_sha1_free(&sha); }

    bool onResponse(int, long long contentLength) override
    {
        if (contentLength > kMaxPackBytes)
        {
            tooLarge = true;
            return false;
        }
        if (contentLength > 0)
            job.total.store(static_cast<int>(contentLength));
        job.phase.store(GuiTexturePackDownload::DownloadJob::Downloading);
        return true;
    }

    bool onData(const unsigned char *data, std::size_t length) override
    {
        if (job.cancel.load())
            return false;
        if (written + static_cast<long long>(length) > kMaxPackBytes)
        {
            tooLarge = true;
            return false;
        }
        if (std::fwrite(data, 1, length, file) != length)
        {
            writeFailed = true;
            return false;
        }
        mbedtls_sha1_update(&sha, data, length);
        written += static_cast<long long>(length);
        job.received.store(static_cast<int>(written));
        return true;
    }

    std::string digest()
    {
        unsigned char out[20];
        mbedtls_sha1_finish(&sha, out);
        return toHex(out, sizeof(out));
    }

    std::FILE *file;
    GuiTexturePackDownload::DownloadJob &job;
    mbedtls_sha1_context sha;
    long long written = 0;
    bool tooLarge = false;
    bool writeFailed = false;
};

void runDownload(std::shared_ptr<GuiTexturePackDownload::DownloadJob> job)
{
    typedef GuiTexturePackDownload::DownloadJob Job;
    PlatformStorage::mkdirs(kPackDir);
    const std::string dlPath = std::string(kPackDir) + "\\" + job->packName + ".dl";
    auto finish = [&](Job::Outcome outcome, const std::string &detail) {
        job->outcome = outcome;
        job->detail = detail;
        job->phase.store(Job::Done);
    };

    std::FILE *file = std::fopen(dlPath.c_str(), "wb");
    if (file == nullptr)
    {
        finish(Job::WriteError, "fopen " + dlPath);
        return;
    }
    std::setvbuf(file, nullptr, _IOFBF, 64 * 1024);
    FileSink sink(file, *job);
    JavaNetwork::HttpStreamOptions options;
    options.connectTimeoutMs = 20000;
    options.idleTimeoutMs = 30000;
    options.cancel = &job->cancel;
    int status = -1;
    const JavaNetwork::HttpResult result = JavaNetwork::httpGetStream(job->url, sink, &status, options);
    const bool closedOk = std::fclose(file) == 0;
    MC_LOG_INFO("texdl", "download %s -> %d (http %d), %ld bytes\n", job->url.c_str(), static_cast<int>(result), status,
        static_cast<long>(sink.written));

    Job::Outcome failure = Job::None;
    std::string detail;
    if (job->cancel.load()) failure = Job::Cancelled;
    else if (sink.tooLarge) failure = Job::TooLarge;
    else if (sink.writeFailed || !closedOk) failure = Job::WriteError;
    else if (result == JavaNetwork::HttpResult::NotFound) failure = Job::NotFound;
    else if (result != JavaNetwork::HttpResult::Ok) failure = Job::NoConnection;
    else if (job->expectedSize > 0 && sink.written != job->expectedSize)
    {
        failure = Job::BadFile;
        detail = "size " + std::to_string(sink.written) + " != " + std::to_string(job->expectedSize);
    }
    else if (!job->sha1.empty())
    {
        const std::string got = sink.digest();
        if (got != job->sha1)
        {
            failure = Job::BadFile;
            detail = "sha1 " + got;
        }
    }
    if (failure != Job::None)
    {
        PlatformStorage::removeFile(dlPath);
        finish(failure, detail);
        return;
    }

    // Convert (or keep) it for the console.
    job->phase.store(Job::Converting);
    TexDl::ConvertInput in;
    in.zipPath = dlPath;
    in.outDir = kPackDir;
    in.packName = job->packName;
    in.fallbackLine1 = job->title;
    in.fallbackLine2 = "Modrinth - " + job->author;
    in.defaultTerrainPng.swap(job->defaultTerrain);
    in.defaultItemsPng.swap(job->defaultItems);
    in.game152 = TEXDL_GAME_152 != 0;
    in.maxCell = 32;
    in.freeKb = []() { return platformHeapFreeKb(); };
    const long freeBefore = platformHeapFreeKb();
    const TexDl::ConvertResult converted = TexDl::convertDownloadedPack(in, job->report, &job->convert, &job->cancel);
    MC_LOG_INFO("texdl", "convert %s: result %d format %s %dx -> %dx, %d files, %d placed, %d skipped (%s) free %ld -> %ld KB\n",
        job->packName.c_str(), static_cast<int>(converted), job->report.format.c_str(), job->report.resolution,
        job->report.cell, job->report.filesWritten, job->report.texturesPlaced, job->report.skipped,
        job->report.detail.c_str(), freeBefore, platformHeapFreeKb());

    switch (converted)
    {
    case TexDl::ConvertResult::KeptZip:
        if (!PlatformStorage::renameFile(dlPath, job->report.outputPath))
        {
            PlatformStorage::removeFile(dlPath);
            finish(Job::WriteError, "rename");
            return;
        }
        job->converted = false;
        finish(Job::Installed, std::string());
        return;
    case TexDl::ConvertResult::MadePak:
        PlatformStorage::removeFile(dlPath);
        job->converted = true;
        finish(Job::Installed, std::string());
        return;
    default:
        break;
    }
    PlatformStorage::removeFile(dlPath);
    switch (converted)
    {
    case TexDl::ConvertResult::NotCompatible: finish(Job::NotCompatible, job->report.detail); break;
    case TexDl::ConvertResult::TooBig: finish(Job::TooBig, job->report.detail); break;
    case TexDl::ConvertResult::NoMemory: finish(Job::NoMemory, job->report.detail); break;
    case TexDl::ConvertResult::WriteError: finish(Job::WriteError, job->report.detail); break;
    case TexDl::ConvertResult::Cancelled: finish(Job::Cancelled, std::string()); break;
    default: finish(Job::BadFile, job->report.detail); break;
    }
}
} // namespace

// ---------------------------------------------------------------- screen --
GuiTexturePackDownload::GuiTexturePackDownload(GuiScreen *parent) : parentScreen(parent)
{
}

GuiTexturePackDownload::~GuiTexturePackDownload()
{
    cancelJobs();
    delete searchField;
    searchField = nullptr;
}

bool GuiTexturePackDownload::isLegacy() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiTexturePackDownload::isSpanish() const
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

bool GuiTexturePackDownload::usesSpecializedMenuNavigation() const
{
    return !searching;
}

int GuiTexturePackDownload::tabCount() const
{
    return query.empty() ? 5 : 6;
}

std::string GuiTexturePackDownload::tabLabel(int t, bool shortLabel) const
{
    const bool es = isSpanish();
    // Tab 0: only packs made for this version; 1..4 the full Modrinth lists.
    if (t == 0)
        return shortLabel ? (es ? "Para " : "For ") + std::string(kGameVersion)
                          : (es ? "Hechos para " : "Made for ") + std::string(kGameVersion);
    --t;
    switch (t)
    {
    case 0: return shortLabel ? (es ? "Populares" : "Popular") : (es ? "M\xC3\xA1s descargados" : "Most downloaded");
    case 1: return shortLabel ? (es ? "Nuevos" : "Newest") : (es ? "M\xC3\xA1s nuevos" : "Newest");
    case 2: return shortLabel ? (es ? "Al d\xC3\xAD" "a" : "Updated") : (es ? "Actualizados" : "Recently updated");
    case 3: return shortLabel ? (es ? "Seguidos" : "Followed") : (es ? "M\xC3\xA1s seguidos" : "Most followed");
    default: return (es ? "Buscar: " : "Search: ") + query;
    }
}

void GuiTexturePackDownload::initGui()
{
    inputLatch = true;
    delete searchField;
    searchField = new GuiTextField(this, fontRenderer, 0, -40, 160, 12, query);
    searchField->setMaxStringLength(32);
    searchField->setFocused(false);
    searching = false;
    if (hits.empty() && pageJob == nullptr)
        requestPage();
}

void GuiTexturePackDownload::onGuiClosed()
{
    VirtualKeyboard::instance().releaseFocus();
    if (searchField != nullptr)
        searchField->setFocused(false);
    searching = false;
    cancelJobs();
    freeIcons();
    // HTTPS state costs nothing once out of the download screens.
    XboxTls::shutdown();
    GuiScreen::onGuiClosed();
}

void GuiTexturePackDownload::cancelJobs()
{
    if (pageJob != nullptr)
        pageJob->cancel.store(true);
    if (versionJob != nullptr)
        versionJob->cancel.store(true);
    if (downloadJob != nullptr && downloadJob->phase.load() != DownloadJob::Done)
        downloadJob->cancel.store(true);
    pageJob.reset();
    versionJob.reset();
}

void GuiTexturePackDownload::freeIcons()
{
    if (mc != nullptr && mc->renderEngine != nullptr)
        for (IconEntry &e : icons)
            if (e.texture >= 0)
                mc->renderEngine->deleteTexture(e.texture);
    icons.clear();
}

void GuiTexturePackDownload::leave()
{
    play("random.back");
    mc->displayGuiScreen(parentScreen);
}

void GuiTexturePackDownload::play(const char *sound, float pitch)
{
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX(sound, 1.0f, pitch);
}

void GuiTexturePackDownload::requestPage()
{
    if (pageJob != nullptr)
        pageJob->cancel.store(true);
    auto job = std::make_shared<PageJob>();
    const std::string facets = std::string("[[\"project_type:resourcepack\"],[\"versions:") + kGameVersion + "\"]]";
    std::string url;
    if (tab == 0)
    {
        // Every hit at once (~250 for each version), filtered on the worker.
        job->fetchAll = true;
        url = "https://api.modrinth.com/v2/search?limit=100&index=downloads&facets=" + TexDl::urlEncode(facets);
    }
    else
    {
        url = "https://api.modrinth.com/v2/search?limit=" + std::to_string(kPageSize) +
            "&offset=" + std::to_string(page * kPageSize) + "&facets=" + TexDl::urlEncode(facets);
        if (tab >= 5)
            url += "&index=relevance&query=" + TexDl::urlEncode(query);
        else
            url += std::string("&index=") + kSortIndex[tab - 1];
    }
    allHits.clear();
    localPages = false;
    job->url = url;
    for (const IconEntry &e : icons)
        job->cachedIcons.push_back(e.projectId);
    pageJob = job;
    pageLoading = true;
    pageError.clear();
    hits.clear();
    selected = 0;
    scroll = 0;

    std::thread([job]() {
        std::vector<unsigned char> body;
        std::vector<TexDl::SearchHit> candidates;
        int status = -1;
        JavaNetwork::HttpResult result =
            JavaNetwork::httpGetToMemory(job->url, body, kMaxJsonBytes, &status, apiOptions(&job->cancel));
        if (result != JavaNetwork::HttpResult::Ok && !job->cancel.load())
        {
            std::vector<unsigned char>().swap(body);
            result = JavaNetwork::httpGetToMemory(job->url, body, kMaxJsonBytes, &status, apiOptions(&job->cancel));
        }
        TexDl::SearchPage page;
        bool parsed = result == JavaNetwork::HttpResult::Ok &&
            TexDl::parseSearch(reinterpret_cast<const char *>(body.data()), body.size(), page);
        std::vector<unsigned char>().swap(body);
        if (job->fetchAll && parsed)
        {
            // Old-format packs only; the rest of the pages one after another.
            std::vector<TexDl::SearchHit> kept;
            const int total = page.totalHits;
            candidates.clear();
            for (int offset = 0;;)
            {
                for (TexDl::SearchHit &h : page.hits)
                {
                    if (!h.newerThanConsole)
                        kept.push_back(std::move(h));
                    else
                        candidates.push_back(std::move(h));
                }
                offset += 100;
                if (offset >= total || offset >= 1000 || job->cancel.load())
                    break;
                page = TexDl::SearchPage();
                result = JavaNetwork::httpGetToMemory(job->url + "&offset=" + std::to_string(offset), body,
                                                      kMaxJsonBytes, &status, apiOptions(&job->cancel));
                parsed = result == JavaNetwork::HttpResult::Ok &&
                    TexDl::parseSearch(reinterpret_cast<const char *>(body.data()), body.size(), page);
                std::vector<unsigned char>().swap(body);
                if (!parsed)
                    break;
            }
            page.hits = std::move(kept);
            page.totalHits = static_cast<int>(page.hits.size());
            parsed = true;
        }
        MC_LOG_INFO("texdl", "search %s -> %d (http %d) parsed %d, %u hits of %d\n", job->url.c_str(),
            static_cast<int>(result), status, parsed ? 1 : 0, static_cast<unsigned>(page.hits.size()), page.totalHits);
        std::vector<std::pair<std::string, std::string>> wanted;
        {
            std::lock_guard<std::mutex> lock(job->mtx);
            job->result = result;
            job->parsed = parsed;
            for (const TexDl::SearchHit &h : page.hits)
                if (std::find(job->cachedIcons.begin(), job->cachedIcons.end(), h.projectId) == job->cachedIcons.end())
                    wanted.emplace_back(h.projectId, h.iconUrl);
            job->page = std::move(page);
            job->verifyTotal = static_cast<int>(candidates.size());
        }
        job->pageDone.store(true);

        // Icons of this page, one at a time; the newest page wins (cancel).
        for (const auto &w : wanted)
        {
            if (job->cancel.load())
                return;
            PageJob::Icon icon;
            icon.projectId = w.first;
            if (!w.second.empty())
            {
                std::vector<unsigned char> bytes;
                JavaNetwork::HttpStreamOptions o = apiOptions(&job->cancel);
                o.accept = "image/png,image/jpeg,image/gif,image/webp,*/*";
                o.connectTimeoutMs = 10000;
                o.idleTimeoutMs = 10000;
                if (JavaNetwork::httpGetToMemory(w.second, bytes, kMaxIconBytes, nullptr, o) == JavaNetwork::HttpResult::Ok)
                {
                    std::string error;
                    if (!TexDl::decodeIcon(bytes, kIconSize, icon.rgba, error))
                        MC_LOG_INFO("texdl", "icon %s: %s\n", w.second.c_str(), error.c_str());
                }
            }
            icon.ready = true;
            std::lock_guard<std::mutex> lock(job->mtx);
            job->iconsOut.push_back(std::move(icon));
        }

        // Tab 0, second pass: ask Modrinth for each multi-version pack's file for
        // this game version and keep it when that file is an old-format one.
        // Three requests at a time, and every answer is remembered on disk
        // (texturepacks\.modrinth-<version>.txt) so the next visit is instant.
        if (candidates.empty())
            return;
        struct VerifyShared
        {
            std::vector<TexDl::SearchHit> cands;
            std::atomic<int> next{0};
            std::mutex cacheMtx;
            std::map<std::string, bool> cache;
            std::string cachePath;
        };
        auto shared = std::make_shared<VerifyShared>();
        shared->cands = std::move(candidates);
        shared->cachePath = std::string(kPackDir) + "\\.modrinth-" + kGameVersion + ".txt";
        if (FILE *f = std::fopen(shared->cachePath.c_str(), "rb"))
        {
            char line[128];
            while (std::fgets(line, sizeof(line), f) != nullptr)
            {
                char id[64] = {0};
                int value = 0;
                if (std::sscanf(line, "%63s %d", id, &value) == 2)
                    shared->cache[id] = value != 0;
            }
            std::fclose(f);
        }
        auto worker = [job, shared]() {
            for (;;)
            {
                if (job->cancel.load())
                    return;
                const int index = shared->next.fetch_add(1);
                if (index >= static_cast<int>(shared->cands.size()))
                    return;
                TexDl::SearchHit cand = shared->cands[static_cast<std::size_t>(index)];
                bool ok = false;
                bool known = false;
                {
                    std::lock_guard<std::mutex> lock(shared->cacheMtx);
                    auto it = shared->cache.find(cand.projectId);
                    if (it != shared->cache.end())
                    {
                        known = true;
                        ok = it->second;
                    }
                }
                if (!known)
                {
                    const std::string vurl = "https://api.modrinth.com/v2/project/" + cand.projectId +
                        "/version?game_versions=" + TexDl::urlEncode(std::string("[\"") + kGameVersion + "\"]");
                    std::vector<unsigned char> vbody;
                    TexDl::ProjectVersion pv;
                    const JavaNetwork::HttpResult r =
                        JavaNetwork::httpGetToMemory(vurl, vbody, kMaxJsonBytes, nullptr, apiOptions(&job->cancel));
                    if (r == JavaNetwork::HttpResult::Ok)
                    {
                        known = true;
                        if (TexDl::parseVersions(reinterpret_cast<const char *>(vbody.data()), vbody.size(), kGameVersion, pv))
                        {
                            int parts[3] = {0, 0, 0};
                            int idx = 0;
                            for (char c : pv.newestGameVersion)
                            {
                                if (c >= '0' && c <= '9') parts[idx] = parts[idx] * 10 + (c - '0');
                                else if (c == '.' && idx < 2) ++idx;
                                else break;
                            }
                            const bool newer = parts[0] > 1 || (parts[0] == 1 && (parts[1] > 5 || (parts[1] == 5 && parts[2] > 2)));
                            ok = !pv.listsModernVersions && !newer;
                            if (ok)
                                MC_LOG_INFO("texdl", "verified %s: file for %s is old format (%s)\n", cand.title.c_str(),
                                            kGameVersion, pv.newestGameVersion.c_str());
                        }
                        std::lock_guard<std::mutex> lock(shared->cacheMtx);
                        shared->cache[cand.projectId] = ok;
                        if (FILE *f = std::fopen(shared->cachePath.c_str(), "ab"))
                        {
                            std::fprintf(f, "%s %d\n", cand.projectId.c_str(), ok ? 1 : 0);
                            std::fclose(f);
                        }
                    }
                }
                PageJob::Icon icon;
                if (ok && !cand.iconUrl.empty() && !job->cancel.load())
                {
                    icon.projectId = cand.projectId;
                    std::vector<unsigned char> bytes;
                    JavaNetwork::HttpStreamOptions o = apiOptions(&job->cancel);
                    o.accept = "image/png,image/jpeg,image/gif,image/webp,*/*";
                    o.connectTimeoutMs = 10000;
                    o.idleTimeoutMs = 10000;
                    if (JavaNetwork::httpGetToMemory(cand.iconUrl, bytes, kMaxIconBytes, nullptr, o) == JavaNetwork::HttpResult::Ok)
                    {
                        std::string error;
                        TexDl::decodeIcon(bytes, kIconSize, icon.rgba, error);
                    }
                    icon.ready = true;
                }
                std::lock_guard<std::mutex> lock(job->mtx);
                ++job->verifyDone;
                if (ok)
                {
                    job->verified.push_back(std::move(cand));
                    if (icon.ready)
                        job->iconsOut.push_back(std::move(icon));
                }
            }
        };
        std::thread second(worker);
        std::thread third(worker);
        worker();
        second.join();
        third.join();
    }).detach();
}

int_t GuiTexturePackDownload::iconFor(const std::string &projectId)
{
    for (IconEntry &e : icons)
        if (e.projectId == projectId)
        {
            e.lastUse = ++iconClock;
            return e.texture;
        }
    return -1;
}

void GuiTexturePackDownload::storeIcon(const std::string &projectId, const std::vector<unsigned char> &rgba)
{
    if (mc == nullptr || mc->renderEngine == nullptr)
        return;
    int_t texture = -1;
    if (rgba.size() == static_cast<std::size_t>(kIconSize) * kIconSize * 4)
    {
        std::unique_ptr<unsigned char[]> pixels(new unsigned char[rgba.size()]);
        std::memcpy(pixels.get(), rgba.data(), rgba.size());
        BufferedImage img(kIconSize, kIconSize, std::move(pixels));
        texture = mc->renderEngine->allocateAndSetupTexture(&img);
    }
    // -1 is kept too ("no icon": placeholder, not fetched again on this page).
    if (icons.size() >= kMaxIcons)
    {
        auto oldest = std::min_element(icons.begin(), icons.end(),
            [](const IconEntry &a, const IconEntry &b) { return a.lastUse < b.lastUse; });
        if (oldest->texture >= 0)
            mc->renderEngine->deleteTexture(oldest->texture);
        icons.erase(oldest);
    }
    IconEntry e;
    e.projectId = projectId;
    e.texture = texture;
    e.lastUse = ++iconClock;
    icons.push_back(e);
}

void GuiTexturePackDownload::pollJobs()
{
    const bool es = isSpanish();
    if (pageJob != nullptr)
    {
        std::lock_guard<std::mutex> lock(pageJob->mtx);
        if (!pageJob->pageTaken && pageJob->pageDone.load())
        {
            pageJob->pageTaken = true;
            pageLoading = false;
            if (pageJob->parsed)
            {
                totalHits = pageJob->page.totalHits;
                const bool fetchAll = pageJob->fetchAll;
                for (const TexDl::SearchHit &h : pageJob->page.hits)
                {
                    auto hit = std::make_shared<Hit>();
                    hit->raw = h;
                    hit->title = cleanText(h.title, 80);
                    if (hit->title.empty())
                        hit->title = h.slug;
                    hit->author = cleanText(h.author, 40);
                    hit->description = cleanText(h.description, 300);
                    if (fetchAll)
                        allHits.push_back(hit);
                    else
                        hits.push_back(hit);
                }
                if (fetchAll)
                {
                    localPages = true;
                    showLocalPage();
                }
                if (hits.empty())
                    pageError = es ? "No hay packs aqu\xC3\xAD." : "No packs here.";
            }
            else if (pageJob->result == JavaNetwork::HttpResult::NoConnection)
                pageError = es ? "Sin conexi\xC3\xB3n a internet (cable de red, DNS)." : "No internet connection (network cable, DNS).";
            else
                pageError = es ? "Modrinth no respondi\xC3\xB3 bien. A: reintentar." : "Modrinth gave a bad answer. A: retry.";
        }
        if (pageJob->fetchAll && pageJob->pageTaken && !pageJob->verified.empty())
        {
            for (const TexDl::SearchHit &h : pageJob->verified)
            {
                auto hit = std::make_shared<Hit>();
                hit->raw = h;
                hit->title = cleanText(h.title, 80);
                if (hit->title.empty())
                    hit->title = h.slug;
                hit->author = cleanText(h.author, 40);
                hit->description = cleanText(h.description, 300);
                allHits.push_back(hit);
            }
            pageJob->verified.clear();
            totalHits = static_cast<int>(allHits.size());
            if (hits.size() < static_cast<std::size_t>(kPageSize))
            {
                const int keep = selected;
                showLocalPage();
                selected = std::min(keep, std::max(0, static_cast<int>(hits.size()) - 1));
                pageError.clear();
            }
        }
        verifyDone = pageJob->verifyDone;
        verifyTotal = pageJob->fetchAll ? pageJob->verifyTotal : 0;
        for (PageJob::Icon &icon : pageJob->iconsOut)
            if (icon.ready && !icon.taken)
            {
                icon.taken = true;
                if (iconFor(icon.projectId) < 0)
                    storeIcon(icon.projectId, icon.rgba);
                std::vector<unsigned char>().swap(icon.rgba);
            }
    }

    if (versionJob != nullptr && versionJob->done.load() && mode == Mode::Detail && statusText.empty())
    {
        if (versionJob->found)
        {
            const TexDl::ProjectVersion &v = versionJob->version;
            if (v.file.size > kMaxPackBytes)
            {
                statusText = (es ? "Demasiado grande para la consola (" : "Too big for the console (") +
                    formatBytes(v.file.size, es) + ")";
                statusColor = static_cast<int_t>(0xFFFF6666u);
            }
            else if (v.listsModernVersions)
            {
                statusText = es ? "Pack multiversi\xC3\xB3n: puede que la consola no lo pueda usar"
                                : "Multi-version pack: the console may not be able to use it";
                statusColor = static_cast<int_t>(0xFFFFCC55u);
            }
            else
            {
                statusText = es ? "Listo para descargar" : "Ready to download";
                statusColor = static_cast<int_t>(0xFF77FF77u);
            }
        }
        else if (versionJob->result == JavaNetwork::HttpResult::NoConnection)
        {
            statusText = es ? "Sin conexi\xC3\xB3n a internet" : "No internet connection";
            statusColor = static_cast<int_t>(0xFFFF6666u);
        }
        else
        {
            statusText = es ? (std::string("No hay archivo para ") + kGameVersion) : (std::string("No file for ") + kGameVersion);
            statusColor = static_cast<int_t>(0xFFFF6666u);
        }
    }

    if (downloadJob != nullptr && mode == Mode::Downloading && downloadJob->phase.load() == DownloadJob::Done)
    {
        const DownloadJob &j = *downloadJob;
        mode = Mode::Finished;
        installedName.clear();
        switch (j.outcome)
        {
        case DownloadJob::Installed:
            installedName = j.packName;
            statusText = es ? "Instalado" : "Installed";
            statusColor = static_cast<int_t>(0xFF77FF77u);
            play("random.levelup", 1.2f);
            break;
        case DownloadJob::NoConnection:
            statusText = es ? "Se perdi\xC3\xB3 la conexi\xC3\xB3n" : "Connection lost";
            break;
        case DownloadJob::NotFound:
            statusText = es ? "El archivo ya no existe en Modrinth" : "The file is gone from Modrinth";
            break;
        case DownloadJob::TooLarge:
            statusText = es ? "Demasiado grande para descargarlo aqu\xC3\xAD" : "Too big to download here";
            break;
        case DownloadJob::WriteError:
            statusText = es ? "No se pudo guardar (\xC2\xBF" "disco lleno?)" : "Could not save it (disk full?)";
            break;
        case DownloadJob::BadFile:
            statusText = es ? "El archivo lleg\xC3\xB3 roto o no es un .zip" : "The file arrived broken or is not a .zip";
            break;
        case DownloadJob::NotCompatible:
            statusText = es ? "No compatible con la consola" : "Not compatible with the console";
            break;
        case DownloadJob::TooBig:
            statusText = es ? "Pack demasiado grande (m\xC3\xA1s de 64x)" : "Pack too big (over 64x)";
            break;
        case DownloadJob::NoMemory:
            statusText = es ? "Falta memoria: prueba desde el men\xC3\xBA principal" : "Not enough memory: try from the main menu";
            break;
        case DownloadJob::Cancelled:
        default:
            statusText = es ? "Descarga cancelada" : "Download cancelled";
            break;
        }
        if (j.outcome != DownloadJob::Installed)
        {
            statusColor = static_cast<int_t>(0xFFFF6666u);
            play("random.break", 1.0f);
        }
    }
}

void GuiTexturePackDownload::openDetail()
{
    if (selected < 0 || selected >= static_cast<int>(hits.size()))
        return;
    detailHit = hits[static_cast<std::size_t>(selected)];
    mode = Mode::Detail;
    statusText.clear();
    if (versionJob != nullptr)
        versionJob->cancel.store(true);
    auto job = std::make_shared<VersionJob>();
    job->url = "https://api.modrinth.com/v2/project/" + TexDl::urlEncode(detailHit->raw.projectId) +
        "/version?game_versions=" + TexDl::urlEncode(std::string("[\"") + kGameVersion + "\"]");
    versionJob = job;
    play("random.click");
    std::thread([job]() {
        std::vector<unsigned char> body;
        int status = -1;
        job->result = JavaNetwork::httpGetToMemory(job->url, body, kMaxJsonBytes, &status, apiOptions(&job->cancel));
        if (job->result == JavaNetwork::HttpResult::Ok)
            job->found = TexDl::parseVersions(reinterpret_cast<const char *>(body.data()), body.size(), kGameVersion,
                job->version);
        MC_LOG_INFO("texdl", "versions %s -> %d (http %d) found %d size %ld\n", job->url.c_str(),
            static_cast<int>(job->result), status, job->found ? 1 : 0, static_cast<long>(job->version.file.size));
        job->done.store(true);
    }).detach();
}

void GuiTexturePackDownload::startDownload()
{
    if (detailHit == nullptr || versionJob == nullptr || !versionJob->done.load() || !versionJob->found)
        return;
    const TexDl::ProjectVersion &v = versionJob->version;
    if (v.file.size > kMaxPackBytes)
        return;

    // A name FATX takes, not used by another pack yet.
    std::string base = TexDl::fatxSafeName(detailHit->title, 34);
    std::string name = base;
    for (int n = 2; n < 100; ++n)
    {
        const std::string root = std::string(kPackDir) + "\\" + name;
        if (!PlatformStorage::exists(root + ".zip") && !PlatformStorage::exists(root + ".pak") &&
            !PlatformStorage::exists(root))
            break;
        name = base + " " + std::to_string(n);
    }

    auto job = std::make_shared<DownloadJob>();
    job->url = v.file.url;
    job->sha1 = v.file.sha1;
    job->expectedSize = v.file.size;
    job->total.store(static_cast<int>(v.file.size));
    job->packName = name;
    job->title = detailHit->title;
    job->author = detailHit->author;
    // The game's own atlases, read here (render thread owns the asset pak).
    job->defaultTerrain = readGameResource("/terrain.png");
    job->defaultItems = readGameResource("/gui/items.png");
    downloadJob = job;
    mode = Mode::Downloading;
    statusText.clear();
    animTicks = 0;
    play("random.click", 1.1f);
    MC_LOG_INFO("texdl", "download start '%s' as '%s', %ld bytes, free %ld KB\n", detailHit->title.c_str(), name.c_str(),
        static_cast<long>(v.file.size), platformHeapFreeKb());
    std::thread([job]() { runDownload(job); }).detach();
}

void GuiTexturePackDownload::useInstalledPack()
{
    if (installedName.empty() || mc == nullptr || mc->texturePackList == nullptr)
        return;
    mc->texturePackList->updateAvailableTexturePacks();
    for (TexturePackBase *pack : mc->texturePackList->getAvailableTexturePacks())
        if (pack != nullptr && static_cast<const std::string &>(pack->texturePackFileName) == installedName && pack->isUsable())
        {
            mc->texturePackList->setTexturePack(pack);
            mc->refreshResources();
            break;
        }
    play("random.click");
    mc->displayGuiScreen(parentScreen);
}

void GuiTexturePackDownload::beginSearch()
{
    if (searchField == nullptr)
        return;
    searching = true;
    searchField->setText(query);
    searchField->setFocused(true);
    play("random.click");
}

void GuiTexturePackDownload::endSearch(bool run)
{
    searching = false;
    inputLatch = true;
    const std::string text = searchField != nullptr ? trimSpaces(searchField->getText()) : std::string();
    if (searchField != nullptr)
        searchField->setFocused(false);
    VirtualKeyboard::instance().releaseFocus();
    if (!run)
        return;
    query = text;
    tab = query.empty() ? 0 : 5;
    page = 0;
    requestPage();
}

void GuiTexturePackDownload::changeTab(int delta)
{
    const int count = tabCount();
    tab = (tab + delta + count) % count;
    page = 0;
    play("random.click");
    requestPage();
}

void GuiTexturePackDownload::changePage(int delta)
{
    const int pages = std::max(1, (totalHits + kPageSize - 1) / kPageSize);
    const int next = page + delta;
    if (next < 0 || next >= pages || pageLoading)
        return;
    page = next;
    play("random.click");
    if (localPages)
        showLocalPage();
    else
        requestPage();
}

void GuiTexturePackDownload::showLocalPage()
{
    hits.clear();
    const std::size_t first = static_cast<std::size_t>(page) * kPageSize;
    for (std::size_t i = first; i < allHits.size() && i < first + kPageSize; ++i)
        hits.push_back(allHits[i]);
    selected = 0;
    scroll = 0;
}

void GuiTexturePackDownload::updateScreen()
{
    GuiScreen::updateScreen();
    ++animTicks;
    if (searching && searchField != nullptr)
    {
        searchField->updateCursorCounter();
        // B on the keyboard unfocuses the field: back to the list.
        if (!searchField->getFocused())
            endSearch(false);
    }
    pollJobs();
}

void GuiTexturePackDownload::keyTyped(char_t c, int_t key)
{
    if (searching && searchField != nullptr)
    {
        if (key == lwjgl::Keyboard::KEY_RETURN || key == lwjgl::Keyboard::KEY_NUMPADENTER)
        {
            endSearch(true);
            return;
        }
        if (key == lwjgl::Keyboard::KEY_ESCAPE || key == 1)
        {
            endSearch(false);
            return;
        }
        searchField->textboxKeyTyped(c, key);
        return;
    }
    if (key == lwjgl::Keyboard::KEY_ESCAPE || key == 1)
    {
        if (mode == Mode::Browse)
            leave();
        else if (mode == Mode::Downloading)
            downloadJob->cancel.store(true);
        else
            mode = Mode::Browse;
        return;
    }
    GuiScreen::keyTyped(c, key);
}

void GuiTexturePackDownload::handleSpecializedMenuInput()
{
    if (searching || VirtualKeyboard::instance().isActive())
        return;
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;
    if (inputLatch)
    {
        // Ignore what was held while the screen (or keyboard) opened.
        if ((pad.held & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK | PLATFORM_TEXT_SHIFT |
                            PLATFORM_TEXT_ENTER | PLATFORM_TEXT_TAB_LEFT | PLATFORM_TEXT_TAB_RIGHT)) == 0)
            inputLatch = false;
        return;
    }
    const std::uint32_t p = pad.pressed;

    switch (mode)
    {
    case Mode::Browse:
    {
        if (p & PLATFORM_TEXT_CLOSE) { leave(); return; }
        if (p & PLATFORM_TEXT_BACK) { beginSearch(); return; }
        if (p & PLATFORM_TEXT_TAB_LEFT) { changeTab(-1); return; }
        if (p & PLATFORM_TEXT_TAB_RIGHT) { changeTab(1); return; }
        if (p & PLATFORM_TEXT_LEFT) { changePage(-1); return; }
        if (p & PLATFORM_TEXT_RIGHT) { changePage(1); return; }
        const int count = static_cast<int>(hits.size());
        if ((p & PLATFORM_TEXT_UP) && count > 0)
        {
            if (selected > 0) { --selected; play("random.focus"); }
            else if (page > 0) { changePage(-1); selected = 0; }
            return;
        }
        if ((p & PLATFORM_TEXT_DOWN) && count > 0)
        {
            if (selected + 1 < count) { ++selected; play("random.focus"); }
            else changePage(1);
            return;
        }
        if (p & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER))
        {
            if (!pageError.empty() && hits.empty())
                requestPage();
            else
                openDetail();
        }
        return;
    }
    case Mode::Detail:
        if (p & PLATFORM_TEXT_CLOSE) { mode = Mode::Browse; play("random.back"); return; }
        if (p & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER))
        {
            if (versionJob != nullptr && versionJob->done.load() && !versionJob->found)
                openDetail(); // retry
            else
                startDownload();
        }
        return;
    case Mode::Downloading:
        if ((p & PLATFORM_TEXT_CLOSE) && downloadJob != nullptr)
        {
            downloadJob->cancel.store(true);
            play("random.back");
        }
        return;
    case Mode::Finished:
        if ((p & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_ENTER)) && !installedName.empty()) { useInstalledPack(); return; }
        if (p & (PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_TYPE))
        {
            mode = installedName.empty() ? Mode::Detail : Mode::Browse;
            if (mode == Mode::Detail)
                statusText.clear(); // re-evaluated from the version job
            play("random.back");
        }
        return;
    }
}

// ------------------------------------------------------------------ draw --
void GuiTexturePackDownload::drawIcon(int_t texture, int_t x, int_t y, int_t size)
{
    if (texture < 0 && mc != nullptr && mc->renderEngine != nullptr)
        texture = mc->renderEngine->getTexture("/gui/unknown_pack.png");
    if (texture >= 0)
    {
        renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        legacyDrawUiTexture(texture, x, y, size, size, 0.0f);
    }
}

void GuiTexturePackDownload::drawHints(const std::string *buttons, const std::string *actions, int_t count)
{
    drawControlHintRow(mc, width, legacyMenuLayout(mc, width, height, 300, 1, 0, 0).hintY, buttons, actions, count);
}

// Backdrop + panel; returns the content rectangle (x, y = first line, w, h).
void GuiTexturePackDownload::drawFrame(int_t &x, int_t &y, int_t &w, int_t &h, int_t rows, int_t headerH, int_t footerH,
    float_t partialTick)
{
    const bool wide = width >= 420;
    const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, wide ? 480 : 310, rows, headerH, footerH,
        height >= 330 ? 26 : 22);
    if (isLegacy())
    {
        legacyDrawMenuBackdrop(mc, width, height, partialTick);
        legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, l.panelH);
    }
    else
    {
        drawDefaultBackground();
        Gui::drawRect(l.panelX, l.panelY, l.panelX + l.panelW, l.panelY + l.panelH, static_cast<int_t>(0xA0000000u));
    }
    x = l.contentX;
    y = l.panelY;
    w = l.contentW;
    h = l.panelH;
}

void GuiTexturePackDownload::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    (void)mouseX;
    (void)mouseY;
    switch (mode)
    {
    case Mode::Browse: drawBrowse(partialTick); break;
    case Mode::Detail: drawDetail(); break;
    case Mode::Downloading: drawProgress(); break;
    case Mode::Finished: drawFinished(); break;
    }
}

void GuiTexturePackDownload::drawBrowse(float_t partialTick)
{
    const bool es = isSpanish();
    const bool legacy = isLegacy();
    const bool wide = width >= 420;
    const bool compact = height < 200;
    const int_t rowH = height >= 330 ? 26 : 22;
    const int_t headerH = 40;
    const int_t footerH = compact ? 12 : (wide ? 14 : 38);
    const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, wide ? 480 : 310, kPageSize, headerH, footerH, rowH);
    if (legacy)
    {
        legacyDrawMenuBackdrop(mc, width, height, partialTick);
        legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, l.panelH);
    }
    else
    {
        drawDefaultBackground();
        Gui::drawRect(l.panelX, l.panelY, l.panelX + l.panelW, l.panelY + l.panelH, static_cast<int_t>(0x90000000u));
    }
    const int_t textColor = legacy ? legacyPanelTextColor() : static_cast<int_t>(0xFFFFFFFFu);
    const int_t greyColor = legacy ? static_cast<int_t>(0xFF606060u) : static_cast<int_t>(0xFFA0A0A0u);

    // Title + page.
    const int_t headY = l.panelY + 6;
    const std::string title = std::string(es ? "Texturas de Modrinth" : "Modrinth texture packs") + " (" + kGameVersion + ")";
    const int pages = std::max(1, (totalHits + kPageSize - 1) / kPageSize);
    // Tab 0 keeps finding packs for a while: say so next to the page count.
    const std::string pageText = (es ? "P\xC3\xA1gina " : "Page ") + std::to_string(page + 1) + "/" + std::to_string(pages) +
        (verifyDone < verifyTotal ? (es ? "  (buscando m\xC3\xA1s " : "  (finding more ") + std::to_string(verifyDone) + "/" +
            std::to_string(verifyTotal) + ")" : std::string());
    const int_t pageW = fontRenderer->getStringWidth(pageText);
    fontRenderer->drawString(legacyFitText(fontRenderer, title, l.contentW - pageW - 8), l.contentX, headY, textColor);
    fontRenderer->drawString(pageText, l.contentX + l.contentW - pageW, headY, greyColor);

    // Tabs: all of them when they fit, else the active one between arrows.
    const int_t tabY = headY + 13;
    const int count = tabCount();
    int_t tabsW = 0;
    for (int t = 0; t < count; ++t)
        tabsW += fontRenderer->getStringWidth(tabLabel(t, true)) + 10 + (t > 0 ? 2 : 0);
    if (tabsW <= l.contentW)
    {
        int_t tx = l.contentX;
        for (int t = 0; t < count; ++t)
        {
            const std::string label = t == 5 ? (es ? "Buscar" : "Search") : tabLabel(t, true);
            const int_t tw = fontRenderer->getStringWidth(label) + 10;
            if (legacy)
                legacyDrawMenuTab(mc, tx, tabY, tw, 14, label, t == tab);
            else
            {
                Gui::drawRect(tx, tabY, tx + tw, tabY + 14, t == tab ? static_cast<int_t>(0xFF606060u) : static_cast<int_t>(0xFF303030u));
                drawCenteredString(fontRenderer, label, tx + tw / 2, tabY + 3, t == tab ? 0xFFFF55 : 0xE0E0E0);
            }
            tx += tw + 2;
        }
    }
    else
    {
        const std::string label = "< " + tabLabel(tab, false) + " >";
        if (legacy)
            legacyDrawMenuTab(mc, l.contentX, tabY, l.contentW, 14, label, true);
        else
            drawCenteredString(fontRenderer, label, l.contentX + l.contentW / 2, tabY + 3, 0xFFFF55);
    }

    const int_t listX = l.contentX;
    const int_t listW = wide ? l.contentW * 60 / 100 : l.contentW;
    const int_t listH = l.visibleRows * l.rowH + (l.visibleRows - 1) * l.rowGap;
    const int_t rowW = listW - 7;
    const int n = static_cast<int>(hits.size());
    if (n == 0)
    {
        std::string msg;
        int_t color = textColor;
        if (pageLoading || verifyDone < verifyTotal)
            msg = (es ? "Cargando" : "Loading") + std::string(static_cast<std::size_t>(animTicks / 5 % 4), '.') +
                (verifyDone < verifyTotal ? " " + std::to_string(verifyDone) + "/" + std::to_string(verifyTotal) : std::string());
        else
        {
            msg = pageError;
            color = static_cast<int_t>(0xFFCC3333u);
        }
        legacyDrawPanelTextWrapped(fontRenderer, msg, l.contentX + l.contentW / 2, l.firstRowY + listH / 2 - 8,
            l.contentW - 8, color, 3);
    }
    else
    {
        selected = std::max(0, std::min(selected, n - 1));
        scroll = legacyMenuKeepVisible(selected, n, l.visibleRows, scroll);
        const int rowsToDraw = std::min(l.visibleRows, n - scroll);
        for (int r = 0; r < rowsToDraw; ++r)
        {
            const int idx = scroll + r;
            const Hit &hit = *hits[static_cast<std::size_t>(idx)];
            const int_t ry = l.rowY(r);
            const bool sel = idx == selected;
            int_t labelColor;
            if (legacy)
                labelColor = legacyDrawMenuButtonFrame(mc, listX, ry, rowW, l.rowH, sel, true);
            else
            {
                Gui::drawRect(listX, ry, listX + rowW, ry + l.rowH, sel ? static_cast<int_t>(0xFF505050u) : static_cast<int_t>(0x60000000u));
                if (sel)
                    Gui::drawRect(listX, ry, listX + rowW, ry + 1, static_cast<int_t>(0xFFFFFFFFu));
                labelColor = sel ? static_cast<int_t>(0xFFFFFF55u) : static_cast<int_t>(0xFFFFFFFFu);
            }
            const int_t iconSize = l.rowH - 4;
            drawIcon(iconFor(hit.raw.projectId), listX + 3, ry + 2, iconSize);
            const int_t tx = listX + 3 + iconSize + 5;
            const int_t tw = listX + rowW - 4 - tx;
            const int_t top = ry + (l.rowH - 18) / 2;
            legacyDrawOptionText(fontRenderer, legacyFitText(fontRenderer, hit.title, tw), tx, top, labelColor);
            const std::string sub = (es ? "de " : "by ") + hit.author + "  \xC2\xB7  " +
                formatCount(hit.raw.downloads, es) + (es ? " descargas" : " downloads");
            legacyDrawOptionText(fontRenderer, legacyFitText(fontRenderer, sub, tw), tx, top + 10,
                sel ? static_cast<int_t>(0xFFE8E8E8u) : static_cast<int_t>(0xFFC0C0C0u));
        }
        if (legacy)
            legacyDrawMenuScrollbar(listX + listW - 5, l.firstRowY, listH, scroll, l.visibleRows, n);

        const Hit &sel = *hits[static_cast<std::size_t>(selected)];
        if (wide)
        {
            const int_t dx = listX + listW + 2;
            const int_t dw = l.contentX + l.contentW - dx;
            const int_t dy = l.firstRowY;
            legacyDrawInsetBox(dx, dy, dw, listH, true);
            drawIcon(iconFor(sel.raw.projectId), dx + 6, dy + 6, 48);
            const int_t ix = dx + 60;
            const int_t iw = dw - 64;
            drawString(fontRenderer, legacyFitText(fontRenderer, sel.title, iw), ix, dy + 8, 0xFFFF88);
            drawString(fontRenderer, legacyFitText(fontRenderer, (es ? "de " : "by ") + sel.author, iw), ix, dy + 19, 0xAAAAAA);
            drawString(fontRenderer, legacyFitText(fontRenderer, formatCount(sel.raw.downloads, es) +
                (es ? " descargas, " : " downloads, ") + formatCount(sel.raw.follows, es) + (es ? " seguidores" : " followers"), iw),
                ix, dy + 30, 0x88DDFF);
            int_t lineY = dy + 60;
            std::string rem = sel.description;
            while (!rem.empty() && lineY + 9 < dy + listH - 4)
            {
                std::string line = fontRenderer->trimStringToWidth(rem, dw - 12);
                if (line.empty())
                    break;
                if (line.size() < rem.size())
                {
                    const std::size_t sp = line.rfind(' ');
                    if (sp != std::string::npos && sp > 4)
                        line = line.substr(0, sp);
                }
                drawString(fontRenderer, line, dx + 6, lineY, 0xDDDDDD);
                rem.erase(0, line.size());
                while (!rem.empty() && rem.front() == ' ')
                    rem.erase(rem.begin());
                lineY += 10;
            }
        }
        else if (!compact)
        {
            const int_t fy = l.footerY;
            legacyDrawInsetBox(listX, fy, l.contentW, 24, true);
            drawString(fontRenderer, legacyFitText(fontRenderer, sel.description, l.contentW - 8), listX + 4, fy + 3, 0xEEEEEE);
            drawString(fontRenderer, legacyFitText(fontRenderer, formatCount(sel.raw.follows, es) +
                (es ? " seguidores" : " followers") + (pageLoading ? (es ? "  \xC2\xB7  cargando..." : "  \xC2\xB7  loading...") : "") +
                (verifyDone < verifyTotal ? (es ? "  \xC2\xB7  revisando m\xC3\xA1s " : "  \xC2\xB7  checking more ") +
                    std::to_string(verifyDone) + "/" + std::to_string(verifyTotal) : std::string()),
                l.contentW - 8), listX + 4, fy + 13, 0x88DDFF);
        }
    }

    // Extra prompts on the panel (the hint row takes four).
    if (!compact)
    {
        const std::string ib[] = { "White", "Black", "D-Pad" };
        const std::string ia2[] = { "", es ? "Listas" : "Lists", es ? "Izq/Der: p\xC3\xA1gina" : "Left/Right: page" };
        if (legacy)
            legacyDrawInlineHints(mc, l.panelX + l.panelW / 2, l.panelY + l.panelH - 12, ib, ia2, 3);
        else
            drawCenteredString(fontRenderer, es ? "White/Black: listas   Izq/Der: p\xC3\xA1gina" : "White/Black: lists   Left/Right: page",
                width / 2, l.panelY + l.panelH - 11, 0xA0A0A0);
    }

    const std::string btns[] = { "A", "X", "D-Pad", "B" };
    const std::string acts[] = { es ? "Ver" : "View", es ? "Buscar" : "Search", es ? "Mover" : "Move", es ? "Volver" : "Back" };
    drawControlHintRow(mc, width, l.hintY, btns, acts, 4);

    // Search box over everything while the keyboard is up.
    if (searching && searchField != nullptr)
    {
        const int_t bw = std::min<int_t>(260, width - 40);
        const int_t bx = (width - bw) / 2;
        const int_t by = l.panelY + 20;
        Gui::drawRect(bx - 4, by - 14, bx + bw + 4, by + 18, static_cast<int_t>(0xE0202020u));
        drawString(fontRenderer, es ? "Buscar packs (Start: buscar, B: cerrar)" : "Search packs (Start: search, B: close)",
            bx, by - 11, 0xFFFFFF);
        legacyDrawTextFieldBox(bx, by, bw, 13, true);
        const std::string text = searchField->getText() + ((animTicks / 6) % 2 == 0 ? "_" : "");
        drawString(fontRenderer, legacyFitText(fontRenderer, text, bw - 6), bx + 3, by + 3, 0xFFFFFF);
    }
}

void GuiTexturePackDownload::drawDetail()
{
    const bool es = isSpanish();
    const bool legacy = isLegacy();
    int_t x, y, w, h;
    drawFrame(x, y, w, h, 7, 40, 34, 0.0f);
    if (detailHit == nullptr)
        return;
    const int_t textColor = legacy ? legacyPanelTextColor() : static_cast<int_t>(0xFFFFFFFFu);
    const Hit &hit = *detailHit;
    const int_t iconSize = height >= 330 ? 48 : 32;
    drawIcon(iconFor(hit.raw.projectId), x, y + 8, iconSize);
    const int_t tx = x + iconSize + 8;
    const int_t tw = w - iconSize - 8;
    fontRenderer->drawString(legacyFitText(fontRenderer, hit.title, tw), tx, y + 9, textColor);
    fontRenderer->drawString(legacyFitText(fontRenderer, (es ? "de " : "by ") + hit.author + "  \xC2\xB7  " +
        formatCount(hit.raw.downloads, es) + (es ? " descargas" : " downloads"), tw), tx, y + 20,
        legacy ? static_cast<int_t>(0xFF606060u) : static_cast<int_t>(0xFFA0A0A0u));

    // Description well.
    const int_t wellY = y + iconSize + 14;
    const int_t buttonH = 18;
    const int_t buttonY = y + h - buttonH - 8;
    const int_t infoH = 34;
    const int_t wellH = std::max<int_t>(24, buttonY - 6 - infoH - wellY);
    legacyDrawInsetBox(x, wellY, w, wellH, true);
    int_t lineY = wellY + 4;
    std::string rem = hit.description;
    while (!rem.empty() && lineY + 9 <= wellY + wellH - 2)
    {
        std::string line = fontRenderer->trimStringToWidth(rem, w - 10);
        if (line.empty())
            break;
        if (line.size() < rem.size())
        {
            const std::size_t sp = line.rfind(' ');
            if (sp != std::string::npos && sp > 4)
                line = line.substr(0, sp);
        }
        drawString(fontRenderer, line, x + 5, lineY, 0xDDDDDD);
        rem.erase(0, line.size());
        while (!rem.empty() && rem.front() == ' ')
            rem.erase(rem.begin());
        lineY += 10;
    }

    // File info + status.
    const int_t infoY = wellY + wellH + 4;
    bool canDownload = false;
    if (versionJob == nullptr || !versionJob->done.load())
    {
        fontRenderer->drawString((es ? "Buscando el archivo para " : "Looking for the file for ") + std::string(kGameVersion) +
            std::string(static_cast<std::size_t>(animTicks / 5 % 4), '.'), x, infoY, textColor);
    }
    else
    {
        if (versionJob->found)
        {
            const TexDl::ProjectVersion &v = versionJob->version;
            std::string line = cleanText(v.name.empty() ? v.versionNumber : v.name, 60) + "  \xC2\xB7  " + formatBytes(v.file.size, es);
            if (!v.newestGameVersion.empty())
                line += std::string("  \xC2\xB7  MC ") + kGameVersion + (v.newestGameVersion != kGameVersion ? " - " + v.newestGameVersion : "");
            fontRenderer->drawString(legacyFitText(fontRenderer, line, w), x, infoY, textColor);
            canDownload = v.file.size <= kMaxPackBytes;
        }
        fontRenderer->drawString(legacyFitText(fontRenderer, statusText, w), x, infoY + 12, statusColor);
    }
    const std::string label = versionJob != nullptr && versionJob->done.load() && !versionJob->found
        ? (es ? "Reintentar" : "Retry")
        : (es ? "Descargar" : "Download");
    const int_t bw = std::min<int_t>(160, w);
    if (legacy)
        legacyDrawMenuButton(mc, x + (w - bw) / 2, buttonY, bw, buttonH, label, true,
            canDownload || (versionJob != nullptr && versionJob->done.load() && !versionJob->found));
    else
    {
        Gui::drawRect(x + (w - bw) / 2, buttonY, x + (w + bw) / 2, buttonY + buttonH, static_cast<int_t>(0xFF505050u));
        drawCenteredString(fontRenderer, label, x + w / 2, buttonY + 5, canDownload ? 0xFFFF55 : 0x808080);
    }

    const std::string btns[] = { "A", "B" };
    const std::string acts[] = { label, es ? "Volver" : "Back" };
    drawHints(btns, acts, 2);
}

void GuiTexturePackDownload::drawProgress()
{
    const bool es = isSpanish();
    const bool legacy = isLegacy();
    int_t x, y, w, h;
    drawFrame(x, y, w, h, 3, 34, 30, 0.0f);
    const int_t textColor = legacy ? legacyPanelTextColor() : static_cast<int_t>(0xFFFFFFFFu);
    if (downloadJob == nullptr)
        return;
    const DownloadJob &j = *downloadJob;
    const int phase = j.phase.load();
    const std::string title = (es ? "Descargando: " : "Downloading: ") + j.title;
    fontRenderer->drawString(legacyFitText(fontRenderer, title, w), x, y + 8, textColor);

    int done = 0;
    int total = 1;
    std::string line;
    if (phase == DownloadJob::Converting)
    {
        done = j.convert.done.load();
        total = std::max(1, j.convert.total.load());
        line = (es ? "Preparando para la consola... " : "Preparing for the console... ") + std::to_string(done * 100 / total) + "%";
    }
    else if (phase == DownloadJob::Connecting)
    {
        line = (es ? "Conectando" : "Connecting") + std::string(static_cast<std::size_t>(animTicks / 5 % 4), '.');
    }
    else
    {
        done = j.received.load();
        total = std::max(1, j.total.load());
        const int seconds = std::max(1, static_cast<int>(animTicks / 20));
        line = formatBytes(done, es) + (es ? " de " : " of ") + formatBytes(total, es) + "  \xC2\xB7  " +
            formatBytes(static_cast<long long>(done) / seconds, es) + "/s";
    }
    if (j.cancel.load())
        line = es ? "Cancelando..." : "Cancelling...";

    const int_t barY = y + 26;
    const int_t barH = 12;
    legacyDrawInsetBox(x, barY, w, barH, true);
    const int_t fill = static_cast<int_t>(static_cast<long long>(w - 4) * std::min(done, total) / total);
    if (fill > 0)
        Gui::drawRect(x + 2, barY + 2, x + 2 + fill, barY + barH - 2,
            phase == DownloadJob::Converting ? static_cast<int_t>(0xFF55AAFFu) : static_cast<int_t>(0xFF55CC55u));
    fontRenderer->drawString(legacyFitText(fontRenderer, line, w), x, barY + barH + 6, textColor);
    fontRenderer->drawString(legacyFitText(fontRenderer, std::string("texturepacks\\") + j.packName, w), x, barY + barH + 18,
        legacy ? static_cast<int_t>(0xFF606060u) : static_cast<int_t>(0xFFA0A0A0u));

    const std::string btns[] = { "B" };
    const std::string acts[] = { es ? "Cancelar" : "Cancel" };
    drawHints(btns, acts, 1);
}

void GuiTexturePackDownload::drawFinished()
{
    const bool es = isSpanish();
    const bool legacy = isLegacy();
    int_t x, y, w, h;
    drawFrame(x, y, w, h, 3, 34, 30, 0.0f);
    const int_t textColor = legacy ? legacyPanelTextColor() : static_cast<int_t>(0xFFFFFFFFu);
    const int_t greyColor = legacy ? static_cast<int_t>(0xFF606060u) : static_cast<int_t>(0xFFA0A0A0u);
    if (downloadJob == nullptr)
        return;
    const DownloadJob &j = *downloadJob;
    const bool ok = !installedName.empty();
    fontRenderer->drawString(legacyFitText(fontRenderer, j.title, w), x, y + 8, textColor);
    fontRenderer->drawString(legacyFitText(fontRenderer, statusText, w), x, y + 24, statusColor);
    std::string line1;
    std::string line2;
    if (ok)
    {
        line1 = std::string("texturepacks\\") + installedName + (j.converted ? ".pak" : ".zip");
        if (j.converted)
            line2 = (es ? "Formato " : "Format ") + j.report.format + ", " + std::to_string(j.report.resolution) + "x -> " +
                std::to_string(j.report.cell) + "x" + (es ? " (convertido en la consola)" : " (converted on the console)");
        else
            line2 = es ? "Se usa tal cual (formato 1.2)" : "Used as it is (1.2 format)";
    }
    else
    {
        if (j.outcome == DownloadJob::NotCompatible)
            line1 = es ? "No trae texturas que esta versi\xC3\xB3n use (p. ej. pack 1.13+)" : "No textures this version uses (e.g. a 1.13+ pack)";
        line2 = j.detail;
    }
    fontRenderer->drawString(legacyFitText(fontRenderer, line1, w), x, y + 40, greyColor);
    fontRenderer->drawString(legacyFitText(fontRenderer, line2, w), x, y + 52, greyColor);

    if (ok)
    {
        const std::string btns[] = { "A", "B" };
        const std::string acts[] = { es ? "Usar ahora" : "Use now", es ? "Volver" : "Back" };
        drawHints(btns, acts, 2);
    }
    else
    {
        const std::string btns[] = { "B" };
        const std::string acts[] = { es ? "Volver" : "Back" };
        drawHints(btns, acts, 1);
    }
}

#else // !XBOX_PLATFORM

// Other platforms have no downloader; the class exists so shared code links.
GuiTexturePackDownload::GuiTexturePackDownload(GuiScreen *parent) : parentScreen(parent) {}
GuiTexturePackDownload::~GuiTexturePackDownload() {}
void GuiTexturePackDownload::initGui() {}
void GuiTexturePackDownload::onGuiClosed() { GuiScreen::onGuiClosed(); }
void GuiTexturePackDownload::updateScreen() { GuiScreen::updateScreen(); }
void GuiTexturePackDownload::drawScreen(int_t x, int_t y, float_t f) { drawDefaultBackground(); GuiScreen::drawScreen(x, y, f); }
void GuiTexturePackDownload::keyTyped(char_t, int_t key)
{
    if (key == 1 && mc != nullptr)
        mc->displayGuiScreen(parentScreen);
}
bool GuiTexturePackDownload::usesSpecializedMenuNavigation() const { return false; }
void GuiTexturePackDownload::handleSpecializedMenuInput() {}

#endif // XBOX_PLATFORM
