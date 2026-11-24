// See XboxLauncherWifi.h.
#include "xbox/tools/XboxLauncherWifi.h"

#include "xbox/XboxXtl.h"
#include "xbox/system/XboxNetwork.h"
#include "xbox/system/XboxTls.h"

#include "mbedtls/sha256.h"

#ifndef XBOX_LAUNCHER_WEB
#define XBOX_LAUNCHER_WEB 0
#endif

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

const char* const kDefaultBase =
    "https://github.com/xeodeo/OptiCraft-Heritage-XBOX-CLASSIC-Edition/releases/download/ultima/";
char kReleaseBase[256];

// Tests: D:\wifi-base.txt replaces the release URL (e.g. http://10.0.2.2:8000/ under xemu).
void loadBase()
{
    std::snprintf(kReleaseBase, sizeof(kReleaseBase), "%s", kDefaultBase);
    HANDLE f = CreateFileA("D:\\wifi-base.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    char buf[255] = "";
    DWORD got = 0;
    ReadFile(f, buf, sizeof(buf) - 1, &got, NULL);
    CloseHandle(f);
    buf[got] = 0;
    for (DWORD i = 0; i < got; ++i)
        if (buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ') buf[i] = 0;
    if (buf[0]) std::snprintf(kReleaseBase, sizeof(kReleaseBase), "%s", buf);
}
const int kTimeoutMs = 20000;

CRITICAL_SECTION g_lock;
LauncherWifi::Progress g_progress;
char g_address[20];
volatile LONG g_finished;
volatile LONG g_started;

struct Lock {
    Lock() { EnterCriticalSection(&g_lock); }
    ~Lock() { LeaveCriticalSection(&g_lock); }
};

void setMessage(const char* fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Lock l;
    std::snprintf(g_progress.message, sizeof(g_progress.message), "%s", buf);
}

// --- HTTP(S) client ---------------------------------------------------------------

struct Conn {
    intptr_t socket = XboxNetwork::kInvalidSocket;
    XboxTls::Session* tls = nullptr;

    int recv(char* buf, int len)
    {
        return tls ? XboxTls::recv(tls, buf, len, kTimeoutMs) : XboxNetwork::tcpRecvTimeout(socket, buf, len, kTimeoutMs);
    }
    bool sendAll(const char* buf, int len)
    {
        if (tls) return XboxTls::send(tls, buf, len, kTimeoutMs) == len;
        int off = 0;
        while (off < len) {
            const int n = XboxNetwork::tcpSendTimeout(socket, buf + off, len - off, kTimeoutMs);
            if (n <= 0) return false;
            off += n;
        }
        return true;
    }
    void close()
    {
        if (tls) XboxTls::close(tls);
        tls = nullptr;
        if (socket != XboxNetwork::kInvalidSocket) XboxNetwork::tcpClose(socket);
        socket = XboxNetwork::kInvalidSocket;
    }
};

bool parseUrl(const std::string& url, bool& https, std::string& host, int& port, std::string& path)
{
    size_t p;
    if (url.compare(0, 8, "https://") == 0) { https = true; p = 8; port = 443; }
    else if (url.compare(0, 7, "http://") == 0) { https = false; p = 7; port = 80; }
    else return false;
    const size_t slash = url.find('/', p);
    host = url.substr(p, slash == std::string::npos ? std::string::npos : slash - p);
    path = slash == std::string::npos ? "/" : url.substr(slash);
    const size_t colon = host.find(':');
    if (colon != std::string::npos) {
        port = std::atoi(host.c_str() + colon + 1);
        host.resize(colon);
    }
    return !host.empty();
}

// Body sink: returns false to abort.
typedef bool (*Sink)(void* ctx, const char* data, int len);

// GET with redirects. `length` gets Content-Length or -1. Returns "" or an error.
std::string httpGet(std::string url, Sink sink, void* ctx, long long* length)
{
    for (int hop = 0; hop < 8; ++hop) {
        bool https;
        std::string host, path;
        int port;
        if (!parseUrl(url, https, host, port, path)) return "URL invalida";
        Conn c;
        c.socket = XboxNetwork::tcpConnectTimeout(host.c_str(), port, kTimeoutMs);
        if (c.socket == XboxNetwork::kInvalidSocket) return "No conecta con " + host;
        if (https) {
            char err[96] = "";
            c.tls = XboxTls::open(c.socket, host.c_str(), kTimeoutMs, err, sizeof(err));
            if (!c.tls) {
                c.close();
                return std::string("HTTPS fallo: ") + err;
            }
        }
        const std::string req = "GET " + path + " HTTP/1.0\r\nHost: " + host +
                                "\r\nUser-Agent: OptiCraft-Launcher\r\nAccept: */*\r\nConnection: close\r\n\r\n";
        if (!c.sendAll(req.data(), static_cast<int>(req.size()))) {
            c.close();
            return "No se pudo enviar la peticion";
        }
        // Headers.
        std::string head;
        char buf[8192];
        size_t bodyAt = std::string::npos;
        while (bodyAt == std::string::npos) {
            const int n = c.recv(buf, sizeof(buf));
            if (n <= 0) { c.close(); return "Conexion cortada"; }
            head.append(buf, n);
            bodyAt = head.find("\r\n\r\n");
            if (head.size() > 32768) { c.close(); return "Respuesta rara"; }
        }
        const std::string body0 = head.substr(bodyAt + 4);
        head.resize(bodyAt + 2);
        const int status = head.size() > 12 ? std::atoi(head.c_str() + 9) : 0;
        auto header = [&](const char* name) -> std::string {
            const size_t nl = strlen(name);
            size_t pos = 0;
            while ((pos = head.find("\r\n", pos)) != std::string::npos) {
                pos += 2;
                if (_strnicmp(head.c_str() + pos, name, nl) == 0 && head[pos + nl] == ':') {
                    size_t v = pos + nl + 1;
                    while (v < head.size() && head[v] == ' ') ++v;
                    return head.substr(v, head.find("\r\n", v) - v);
                }
            }
            return std::string();
        };
        if (status >= 300 && status < 400) {
            const std::string loc = header("Location");
            c.close();
            if (loc.empty()) return "Redireccion sin destino";
            if (loc[0] == '/') url = (https ? "https://" : "http://") + host + loc;
            else url = loc;
            continue;
        }
        if (status != 200) {
            c.close();
            char e[48];
            std::snprintf(e, sizeof(e), "El servidor dio %d", status);
            return e;
        }
        const std::string cl = header("Content-Length");
        const long long expected = cl.empty() ? -1 : _atoi64(cl.c_str());
        if (length) *length = expected;
        long long got = 0;
        if (!body0.empty()) {
            if (!sink(ctx, body0.data(), static_cast<int>(body0.size()))) { c.close(); return "Cancelado"; }
            got += body0.size();
        }
        while (expected < 0 || got < expected) {
            const int n = c.recv(buf, sizeof(buf));
            if (n == 0 && expected < 0) break;
            if (n <= 0) { c.close(); return "Descarga cortada"; }
            if (!sink(ctx, buf, n)) { c.close(); return "Cancelado"; }
            got += n;
        }
        c.close();
        return std::string();
    }
    return "Demasiadas redirecciones";
}

bool memorySink(void* ctx, const char* data, int len)
{
    std::string& s = *static_cast<std::string*>(ctx);
    if (s.size() + len > 256 * 1024) return false;
    s.append(data, len);
    return true;
}

struct FileSink {
    HANDLE file;
    mbedtls_sha256_context sha;
    unsigned long long written;
};

bool fileSink(void* ctx, const char* data, int len)
{
    FileSink& f = *static_cast<FileSink*>(ctx);
    DWORD out = 0;
    if (!WriteFile(f.file, data, len, &out, NULL) || out != static_cast<DWORD>(len)) return false;
    mbedtls_sha256_update(&f.sha, reinterpret_cast<const unsigned char*>(data), len);
    f.written += len;
    Lock l;
    g_progress.bytes += len;
    return true;
}

void toHex(const unsigned char* d, int n, char* out)
{
    static const char* k = "0123456789abcdef";
    for (int i = 0; i < n; ++i) {
        out[i * 2] = k[d[i] >> 4];
        out[i * 2 + 1] = k[d[i] & 15];
    }
    out[n * 2] = 0;
}

// SHA-256 of a file on disk, or "" if it cannot be read.
std::string fileSha(const std::string& path, unsigned long long expectedSize)
{
    HANDLE f = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return std::string();
    if (GetFileSize(f, NULL) != expectedSize) {
        CloseHandle(f);
        return std::string();
    }
    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0);
    static char buf[64 * 1024];
    DWORD got = 0;
    while (ReadFile(f, buf, sizeof(buf), &got, NULL) && got > 0)
        mbedtls_sha256_update(&sha, reinterpret_cast<unsigned char*>(buf), got);
    CloseHandle(f);
    unsigned char d[32];
    mbedtls_sha256_finish(&sha, d);
    mbedtls_sha256_free(&sha);
    char hex[65];
    toHex(d, 32, hex);
    return hex;
}

// --- install ------------------------------------------------------------------------

struct Entry {
    std::string path;    // relative, with '\'
    unsigned long long size;
    std::string sha;
    std::string asset;
};

// "1.5.2" for "1.5.2\OptiCraft.xbe"; "" for files beside the launcher (default.xbe, data\...).
std::string folderOf(const std::string& path)
{
    const size_t slash = path.find('\\');
    if (slash == std::string::npos) return std::string();
    const std::string first = path.substr(0, slash);
    return first == "data" ? std::string() : first;
}

std::vector<Entry> parseManifest(const std::string& manifest, char* build, int buildSize)
{
    std::vector<Entry> entries;
    size_t pos = 0;
    while (pos < manifest.size()) {
        size_t nl = manifest.find('\n', pos);
        if (nl == std::string::npos) nl = manifest.size();
        std::string line = manifest.substr(pos, nl - pos);
        pos = nl + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line.empty()) continue;
        if (line[0] == '#') {
            if (build && line.compare(0, 8, "# build ") == 0) std::snprintf(build, buildSize, "%s", line.c_str() + 8);
            continue;
        }
        char path[200], sha[80], asset[200];
        unsigned long long size = 0;
        if (std::sscanf(line.c_str(), "%199s %llu %79s %199s", path, &size, sha, asset) != 4) continue;
        Entry e;
        e.path = path;
        for (char& ch : e.path)
            if (ch == '/') ch = '\\';
        if (e.path.find("..") != std::string::npos || e.path[0] == '\\' || e.path.find(':') != std::string::npos) continue;
        e.size = size;
        e.sha = sha;
        e.asset = asset;
        entries.push_back(e);
    }
    return entries;
}

void makeDirs(const std::string& fullPath)
{
    for (size_t i = 3; i < fullPath.size(); ++i)
        if (fullPath[i] == '\\') CreateDirectoryA(fullPath.substr(0, i).c_str(), NULL);
}

bool canWrite(const std::string& root)
{
    const std::string probe = root + "__wifi_prueba.tmp";
    HANDLE f = CreateFileA(probe.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return false;
    CloseHandle(f);
    DeleteFileA(probe.c_str());
    return true;
}

void deleteTree(const std::string& dir)
{
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.cFileName[0] == '.') continue;
            const std::string path = dir + fd.cFileName;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) deleteTree(path + "\\");
            else DeleteFileA(path.c_str());
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryA(dir.substr(0, dir.size() - 1).c_str());
}

// D: is the launcher's own folder. Run from a disc (or xemu's ISO) it is
// read-only: then the files go to T: so the download can still be tried.
// Tests: D:\wifi-limpiar.txt empties that T: folder first (a "nothing installed" console).
std::string installRoot(bool* test)
{
    static bool cleaned = false;
    if (!cleaned) {
        cleaned = true;
        if (GetFileAttributesA("D:\\wifi-limpiar.txt") != (DWORD)-1 && !canWrite("D:\\"))
            deleteTree("T:\\wifi-prueba\\");
    }
    if (canWrite("D:\\")) {
        if (test) *test = false;
        return "D:\\";
    }
    CreateDirectoryA("T:\\wifi-prueba", NULL);
    if (test) *test = true;
    return "T:\\wifi-prueba\\";
}

// What is known to be installed: "<path> <size> <sha256>" per file, in
// <root>instalado.txt, so checking for updates does not hash every file.
struct Installed {
    std::vector<std::string> path, sha;
    std::vector<unsigned long long> size;

    int find(const std::string& p) const
    {
        for (size_t i = 0; i < path.size(); ++i)
            if (_stricmp(path[i].c_str(), p.c_str()) == 0) return static_cast<int>(i);
        return -1;
    }
    void set(const Entry& e)
    {
        const int i = find(e.path);
        if (i >= 0) {
            sha[i] = e.sha;
            size[i] = e.size;
        } else {
            path.push_back(e.path);
            sha.push_back(e.sha);
            size.push_back(e.size);
        }
    }
    void load(const std::string& root)
    {
        HANDLE f = CreateFileA((root + "instalado.txt").c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (f == INVALID_HANDLE_VALUE) return;
        const DWORD n = GetFileSize(f, NULL);
        std::string text(n, '\0');
        DWORD got = 0;
        ReadFile(f, &text[0], n, &got, NULL);
        CloseHandle(f);
        size_t pos = 0;
        while (pos < text.size()) {
            size_t nl = text.find('\n', pos);
            if (nl == std::string::npos) nl = text.size();
            char p[200], s[80];
            unsigned long long z = 0;
            if (std::sscanf(text.substr(pos, nl - pos).c_str(), "%199s %llu %79s", p, &z, s) == 3) {
                Entry e;
                e.path = p;
                e.size = z;
                e.sha = s;
                set(e);
            }
            pos = nl + 1;
        }
    }
    void save(const std::string& root) const
    {
        std::string text;
        for (size_t i = 0; i < path.size(); ++i) {
            char line[320];
            std::snprintf(line, sizeof(line), "%s %llu %s\n", path[i].c_str(), size[i], sha[i].c_str());
            text += line;
        }
        HANDLE f = CreateFileA((root + "instalado.txt").c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if (f == INVALID_HANDLE_VALUE) return;
        DWORD out = 0;
        WriteFile(f, text.data(), static_cast<DWORD>(text.size()), &out, NULL);
        CloseHandle(f);
    }
    // Same file as the manifest entry? Uses the record when the size on disk
    // still matches; hashes the file only when there is no record. `learn`
    // stores what a hash found.
    bool matches(const std::string& root, const Entry& e, bool* learned)
    {
        HANDLE f = CreateFileA((root + e.path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (f == INVALID_HANDLE_VALUE) return false;
        const DWORD onDisk = GetFileSize(f, NULL);
        CloseHandle(f);
        if (onDisk != e.size) return false;
        const int i = find(e.path);
        if (i >= 0 && size[i] == e.size) return sha[i] == e.sha;
        if (fileSha(root + e.path, e.size) != e.sha) return false;
        set(e);
        if (learned) *learned = true;
        return true;
    }
};

void finish(LauncherWifi::State st)
{
    {
        Lock l;
        g_progress.state = st;
    }
    InterlockedExchange(&g_finished, 1);
    XboxTls::shutdown();
}

// Result of the last update check, per version folder.
const int kMaxFolders = 8;
struct Check {
    LauncherWifi::CheckState state;
    char folder[kMaxFolders][24];
    bool outdated[kMaxFolders];
    unsigned long long bytes[kMaxFolders];
    int count;
    bool launcherOutdated;
    char build[64];
};
Check g_check;

DWORD WINAPI checkThread(void*)
{
    {
        Lock l;
        g_check.state = LauncherWifi::CheckState::Checking;
    }
    std::string manifest;
    const std::string err = httpGet(std::string(kReleaseBase) + "manifest.txt", memorySink, &manifest, nullptr);
    XboxTls::shutdown();
    if (!err.empty()) {
        Lock l;
        g_check.state = LauncherWifi::CheckState::Failed;
        return 0;
    }
    Check c;
    memset(&c, 0, sizeof(c));
    const std::vector<Entry> entries = parseManifest(manifest, c.build, sizeof(c.build));
    const std::string root = installRoot(nullptr);
    Installed inst;
    inst.load(root);
    bool learned = false;
    for (const Entry& e : entries) {
        const std::string folder = folderOf(e.path);
        const bool ok = inst.matches(root, e, &learned);
        if (folder.empty()) {
            if (!ok) c.launcherOutdated = true;
            continue;
        }
        int i = 0;
        while (i < c.count && strcmp(c.folder[i], folder.c_str()) != 0) ++i;
        if (i == c.count) {
            if (c.count == kMaxFolders) continue;
            std::snprintf(c.folder[c.count++], sizeof(c.folder[0]), "%s", folder.c_str());
        }
        if (!ok) c.outdated[i] = true;
        c.bytes[i] += e.size;
    }
    if (learned) inst.save(root);
    c.state = LauncherWifi::CheckState::Done;
    Lock l;
    g_check = c;
    return 0;
}

DWORD WINAPI installThread(void*)
{
    char version[16];
    {
        Lock l;
        std::snprintf(version, sizeof(version), "%s", g_progress.version);
    }
    const bool all = strcmp(version, "todo") == 0;
    if (!XboxNetwork::hasLink()) {
        setMessage("No network: plug the Xbox into your router and try again.");
        finish(LauncherWifi::State::Failed);
        return 0;
    }

    setMessage("Getting the file list...");
    std::string manifest;
    std::string err = httpGet(std::string(kReleaseBase) + "manifest.txt", memorySink, &manifest, nullptr);
    if (!err.empty()) {
        setMessage("Could not get the file list: %s", err.c_str());
        finish(LauncherWifi::State::Failed);
        return 0;
    }
    char build[64] = "";
    std::vector<Entry> entries;
    for (const Entry& e : parseManifest(manifest, build, sizeof(build)))
        if (all || folderOf(e.path) == version) entries.push_back(e);
    {
        Lock l;
        std::snprintf(g_progress.build, sizeof(g_progress.build), "%s", build);
    }
    if (entries.empty()) {
        setMessage("The release has no files for %s.", version);
        finish(LauncherWifi::State::Failed);
        return 0;
    }

    bool test = false;
    const std::string root = installRoot(&test);
    Installed inst;
    inst.load(root);

    // Skip what is already installed and identical.
    setMessage("Checking what you already have...");
    std::vector<Entry> todo;
    unsigned long long total = 0;
    for (const Entry& e : entries) {
        if (inst.matches(root, e, nullptr)) continue;
        todo.push_back(e);
        total += e.size;
    }
    {
        Lock l;
        g_progress.fileCount = static_cast<int>(todo.size());
        g_progress.total = static_cast<unsigned>(total);
        g_progress.bytes = 0;
    }
    if (todo.empty()) {
        inst.save(root);
        setMessage("You already have the newest %s. Nothing to download.", all ? "build" : version);
        finish(LauncherWifi::State::Done);
        return 0;
    }

    // Download every file next to its final name, checked.
    for (size_t i = 0; i < todo.size(); ++i) {
        const Entry& e = todo[i];
        {
            Lock l;
            g_progress.fileIndex = static_cast<int>(i) + 1;
            std::snprintf(g_progress.file, sizeof(g_progress.file), "%s", e.path.c_str());
        }
        setMessage("Downloading %s (%d of %d)", e.path.c_str(), static_cast<int>(i) + 1, static_cast<int>(todo.size()));
        const std::string part = root + e.path + ".part";
        makeDirs(part);
        FileSink fs;
        fs.file = CreateFileA(part.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if (fs.file == INVALID_HANDLE_VALUE) {
            setMessage("Cannot write %s (disk full?)", e.path.c_str());
            finish(LauncherWifi::State::Failed);
            return 0;
        }
        fs.written = 0;
        mbedtls_sha256_init(&fs.sha);
        mbedtls_sha256_starts(&fs.sha, 0);
        long long length = -1;
        err = httpGet(std::string(kReleaseBase) + e.asset, fileSink, &fs, &length);
        CloseHandle(fs.file);
        unsigned char d[32];
        mbedtls_sha256_finish(&fs.sha, d);
        mbedtls_sha256_free(&fs.sha);
        char hex[65];
        toHex(d, 32, hex);
        if (err.empty() && (fs.written != e.size || e.sha != hex))
            err = "the file arrived damaged";
        if (!err.empty()) {
            DeleteFileA(part.c_str());
            setMessage("%s failed: %s. Nothing was changed.", e.path.c_str(), err.c_str());
            finish(LauncherWifi::State::Failed);
            return 0;
        }
    }

    // All good: swap them in.
    setMessage("Installing...");
    for (const Entry& e : todo) {
        const std::string dst = root + e.path;
        const std::string bak = dst + ".bak";
        DeleteFileA(bak.c_str());
        MoveFileA(dst.c_str(), bak.c_str());
        if (!MoveFileA((dst + ".part").c_str(), dst.c_str())) {
            MoveFileA(bak.c_str(), dst.c_str());
            setMessage("Could not replace %s.", e.path.c_str());
            inst.save(root);
            finish(LauncherWifi::State::Failed);
            return 0;
        }
        DeleteFileA(bak.c_str());
        inst.set(e);
    }
    inst.save(root);
    {
        // The check result is stale now: whatever was installed is current.
        Lock l;
        for (int i = 0; i < g_check.count; ++i)
            if (all || strcmp(g_check.folder[i], version) == 0) g_check.outdated[i] = false;
        if (all) g_check.launcherOutdated = false;
    }
    if (test)
        setMessage("Test OK: %d files downloaded and checked into T: (a disc cannot be updated).",
                   static_cast<int>(todo.size()));
    else
        setMessage("Done: %s updated (%d files).", all ? "everything" : version, static_cast<int>(todo.size()));
    finish(LauncherWifi::State::Done);
    return 0;
}

// --- network thread ------------------------------------------------------------------

#if XBOX_LAUNCHER_WEB
// Optional page for a phone (off: the user wants the console to do it all by itself).

const char* const kVersions[] = {"1.5.2", "1.2.5", "todo"};

void sendText(SOCKET s, const char* status, const char* type, const std::string& body, const char* extra = "")
{
    char head[256];
    std::snprintf(head, sizeof(head),
                  "HTTP/1.0 %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nCache-Control: no-store\r\n"
                  "Access-Control-Allow-Origin: *\r\n%sConnection: close\r\n\r\n",
                  status, type, static_cast<unsigned>(body.size()), extra);
    ::send(s, head, static_cast<int>(strlen(head)), 0);
    size_t off = 0;
    while (off < body.size()) {
        const int n = ::send(s, body.data() + off, static_cast<int>(body.size() - off), 0);
        if (n <= 0) break;
        off += n;
    }
}

std::string htmlEscape(const char* s)
{
    std::string o;
    for (; *s; ++s) {
        if (*s == '<') o += "&lt;";
        else if (*s == '>') o += "&gt;";
        else if (*s == '&') o += "&amp;";
        else o += *s;
    }
    return o;
}

std::string statusJson()
{
    LauncherWifi::Progress p;
    LauncherWifi::progress(p);
    const char* st[] = {"idle", "running", "done", "failed"};
    char buf[512];
    std::snprintf(buf, sizeof(buf),
                  "{\"state\":\"%s\",\"version\":\"%s\",\"file\":%d,\"files\":%d,\"bytes\":%u,\"total\":%u,"
                  "\"message\":\"%s\",\"build\":\"%s\"}",
                  st[static_cast<int>(p.state)], p.version, p.fileIndex, p.fileCount, p.bytes, p.total, p.message,
                  p.build);
    return buf;
}

std::string page()
{
    LauncherWifi::Progress p;
    LauncherWifi::progress(p);
    const bool running = p.state == LauncherWifi::State::Running;
    const int pct = p.total ? static_cast<int>(100.0 * p.bytes / p.total) : 0;
    std::string h =
        "<!doctype html><html lang=es><head><meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'><title>OptiCraft en tu Xbox</title>";
    if (running) h += "<meta http-equiv=refresh content=2>";
    h += "<style>body{background:#1d1f21;color:#eee;font-family:sans-serif;margin:0;padding:18px;max-width:560px}"
         "h1{font-size:22px;color:#9be37a}a.b{display:block;margin:10px 0;padding:16px;background:#6d6d6d;color:#fff;"
         "text-decoration:none;text-align:center;font-size:18px;border:2px solid #000;box-shadow:inset 2px 2px #aaa,"
         "inset -2px -2px #444}a.b.off{opacity:.4;pointer-events:none}.bar{height:18px;background:#000;border:2px solid #555}"
         ".bar i{display:block;height:100%;background:#5bb33a}.m{background:#000a;padding:10px;margin:12px 0}"
         "small{color:#999}</style></head><body><h1>OptiCraft &middot; instalar por WiFi</h1>";
    h += "<div class=m>" + htmlEscape(p.message[0] ? p.message : "Elige que version quieres instalar o actualizar.") + "</div>";
    if (running) {
        char bar[200];
        std::snprintf(bar, sizeof(bar), "<div class=bar><i style='width:%d%%'></i></div><p>%d%% &middot; archivo %d de %d</p>",
                      pct, pct, p.fileIndex, p.fileCount);
        h += bar;
    }
    for (const char* v : kVersions) {
        h += std::string("<a class='b") + (running ? " off" : "") + "' href='/instalar?v=" + v + "'>";
        h += strcmp(v, "todo") == 0 ? std::string("Instalar / actualizar TODO") : std::string("Instalar / actualizar ") + v;
        h += "</a>";
    }
    if (p.build[0]) h += "<p><small>Ultima build: " + htmlEscape(p.build) + "</small></p>";
    h += "<p><small>Se baja de GitHub directo a la consola. Si algo falla no se toca lo que ya tienes.</small></p>"
         "</body></html>";
    return h;
}

void handle(SOCKET c)
{
    char req[2048];
    int len = 0;
    while (len < static_cast<int>(sizeof(req)) - 1) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(c, &set);
        timeval tv = {3, 0};
        if (select(0, &set, NULL, NULL, &tv) <= 0) break;
        const int n = ::recv(c, req + len, sizeof(req) - 1 - len, 0);
        if (n <= 0) break;
        len += n;
        req[len] = 0;
        if (strstr(req, "\r\n\r\n")) break;
    }
    req[len] = 0;
    char path[512] = "/";
    std::sscanf(req, "GET %511s", path);

    if (strncmp(path, "/instalar", 9) == 0) {
        const char* v = strstr(path, "v=");
        char version[16] = "";
        if (v) {
            int i = 0;
            for (v += 2; *v && *v != '&' && i < 15; ++v) version[i++] = *v;
            version[i] = 0;
        }
        bool known = false;
        for (const char* k : kVersions) known |= strcmp(k, version) == 0;
        if (known) LauncherWifi::startInstall(version);
        sendText(c, "303 See Other", "text/plain", "", "Location: /\r\n");
    } else if (strncmp(path, "/estado", 7) == 0) {
        sendText(c, "200 OK", "application/json", statusJson());
    } else if (strcmp(path, "/") == 0 || strncmp(path, "/?", 2) == 0) {
        sendText(c, "200 OK", "text/html; charset=utf-8", page());
    } else {
        sendText(c, "404 Not Found", "text/plain", "no");
    }
    shutdown(c, SD_SEND);
    closesocket(c);
}

#endif

DWORD WINAPI serverThread(void*)
{
    if (!XboxNetwork::initialize()) return 0;
    for (;;) {
        // Wait for an address (cable plugged in later, DHCP late).
        XNADDR addr;
        const DWORD st = XNetGetTitleXnAddr(&addr);
        if (st != XNET_GET_XNADDR_PENDING && st != XNET_GET_XNADDR_NONE) {
            Lock l;
            const unsigned char* b = reinterpret_cast<const unsigned char*>(&addr.ina);
            std::snprintf(g_address, sizeof(g_address), "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
            break;
        }
        Sleep(1000);
    }
    LauncherWifi::startCheck();
#if XBOX_LAUNCHER_WEB
    SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) return 0;
    sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(80);
    sa.sin_addr.s_addr = INADDR_ANY;
    if (bind(srv, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) != 0 || listen(srv, 4) != 0) {
        closesocket(srv);
        Lock l;
        g_address[0] = 0;
        return 0;
    }
    for (;;) {
        SOCKET c = accept(srv, NULL, NULL);
        if (c == INVALID_SOCKET) {
            Sleep(200);
            continue;
        }
        handle(c);
    }
#else
    return 0;
#endif
}

} // namespace

namespace LauncherWifi
{
void start()
{
    if (InterlockedExchange(&g_started, 1)) return;
    InitializeCriticalSection(&g_lock);
    memset(&g_progress, 0, sizeof(g_progress));
    loadBase();
    HANDLE t = CreateThread(NULL, 64 * 1024, serverThread, NULL, 0, NULL);
    if (t) CloseHandle(t);
}

void address(char* out, int size)
{
    Lock l;
    std::snprintf(out, size, "%s", g_address);
}

void progress(Progress& out)
{
    Lock l;
    out = g_progress;
}

bool startInstall(const char* version)
{
    {
        Lock l;
        if (g_progress.state == State::Running) return false;
        memset(&g_progress, 0, sizeof(g_progress));
        g_progress.state = State::Running;
        std::snprintf(g_progress.version, sizeof(g_progress.version), "%s", version);
        std::snprintf(g_progress.message, sizeof(g_progress.message), "Empezando...");
    }
    // TLS + SHA-256 + a 64 KB read buffer: give it room.
    HANDLE t = CreateThread(NULL, 256 * 1024, installThread, NULL, 0, NULL);
    if (!t) {
        Lock l;
        g_progress.state = State::Failed;
        std::snprintf(g_progress.message, sizeof(g_progress.message), "No se pudo crear el hilo.");
        return false;
    }
    CloseHandle(t);
    return true;
}

void startCheck()
{
    {
        Lock l;
        if (g_check.state == CheckState::Checking || g_progress.state == State::Running) return;
        g_check.state = CheckState::Checking;
    }
    HANDLE t = CreateThread(NULL, 256 * 1024, checkThread, NULL, 0, NULL);
    if (t) CloseHandle(t);
}

CheckState checkState()
{
    Lock l;
    return g_check.state;
}

bool updateAvailable(const char* folder)
{
    Lock l;
    if (g_check.state != CheckState::Done) return false;
    if (!folder || !*folder) {
        for (int i = 0; i < g_check.count; ++i)
            if (g_check.outdated[i]) return true;
        return g_check.launcherOutdated;
    }
    for (int i = 0; i < g_check.count; ++i)
        if (_stricmp(g_check.folder[i], folder) == 0) return g_check.outdated[i];
    return true;   // the release has it and this console does not
}

int releaseFolders(char (*names)[24], unsigned* megabytes, int max)
{
    Lock l;
    if (g_check.state != CheckState::Done) return 0;
    int n = 0;
    for (int i = 0; i < g_check.count && n < max; ++i, ++n) {
        std::snprintf(names[n], 24, "%s", g_check.folder[i]);
        if (megabytes) megabytes[n] = static_cast<unsigned>((g_check.bytes[i] + 1048575) / 1048576);
    }
    return n;
}

void installRootPath(char* out, int size)
{
    std::snprintf(out, size, "%s", installRoot(nullptr).c_str());
}

void latestBuild(char* out, int size)
{
    Lock l;
    std::snprintf(out, size, "%s", g_check.build);
}

void clear()
{
    Lock l;
    if (g_progress.state != State::Running) g_progress.state = State::Idle;
}

bool takeFinished()
{
    return InterlockedExchange(&g_finished, 0) != 0;
}
} // namespace LauncherWifi
