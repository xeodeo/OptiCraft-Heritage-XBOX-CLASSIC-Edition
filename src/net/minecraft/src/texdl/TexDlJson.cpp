#include "texdl/TexDlJson.h"

#include <cstdlib>
#include <cstring>

namespace TexDl
{
namespace
{
// Pull reader over one JSON document. Every call leaves `p` after what it
// read; any syntax error sets `ok` false and the callers stop.
class JsonReader
{
public:
    JsonReader(const char *data, std::size_t size) : p(data), end(data + size), ok(data != nullptr) {}

    bool good() const { return ok; }

    char peek()
    {
        skipSpace();
        return p < end ? *p : '\0';
    }

    bool beginObject() { return expect('{'); }
    bool beginArray() { return expect('['); }

    // Next key of the object being read; false (and the '}' consumed) at its end.
    bool nextKey(std::string &key, bool &first)
    {
        if (!ok)
            return false;
        skipSpace();
        if (p < end && *p == '}')
        {
            ++p;
            return false;
        }
        if (!first && !expect(','))
            return false;
        first = false;
        if (!readString(key) || !expect(':'))
        {
            ok = false;
            return false;
        }
        return true;
    }

    // True while the array being read has another element (the ',' consumed).
    bool nextElement(bool &first)
    {
        if (!ok)
            return false;
        skipSpace();
        if (p < end && *p == ']')
        {
            ++p;
            return false;
        }
        if (!first && !expect(','))
            return false;
        first = false;
        return true;
    }

    bool readString(std::string &out)
    {
        out.clear();
        if (!expect('"'))
            return false;
        while (p < end)
        {
            const char c = *p++;
            if (c == '"')
                return true;
            if (c != '\\')
            {
                out.push_back(c);
                continue;
            }
            if (p >= end)
                break;
            const char e = *p++;
            switch (e)
            {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u':
            {
                unsigned cp = 0;
                if (!readHex4(cp))
                    return fail();
                if (cp >= 0xD800 && cp <= 0xDBFF && end - p >= 6 && p[0] == '\\' && p[1] == 'u')
                {
                    p += 2;
                    unsigned low = 0;
                    if (!readHex4(low))
                        return fail();
                    if (low >= 0xDC00 && low <= 0xDFFF)
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    else
                        cp = '?';
                }
                else if (cp >= 0xD800 && cp <= 0xDFFF)
                {
                    cp = '?';
                }
                appendUtf8(out, cp);
                break;
            }
            default:
                return fail();
            }
        }
        return fail();
    }

    bool readNumber(double &out)
    {
        skipSpace();
        const char *start = p;
        while (p < end && (*p == '-' || *p == '+' || *p == '.' || *p == 'e' || *p == 'E' || (*p >= '0' && *p <= '9')))
            ++p;
        if (p == start || p - start > 40)
            return fail();
        char buffer[48];
        std::memcpy(buffer, start, static_cast<std::size_t>(p - start));
        buffer[p - start] = '\0';
        out = std::strtod(buffer, nullptr);
        return true;
    }

    // Integer fields (downloads, size): digits only, no double rounding.
    bool readInt64(std::int64_t &out)
    {
        skipSpace();
        if (p < end && (*p == '-' || (*p >= '0' && *p <= '9')))
        {
            const char *start = p;
            bool negative = false;
            if (*p == '-')
            {
                negative = true;
                ++p;
            }
            std::int64_t value = 0;
            while (p < end && *p >= '0' && *p <= '9')
            {
                value = value * 10 + (*p - '0');
                ++p;
            }
            if (p < end && (*p == '.' || *p == 'e' || *p == 'E'))
            {
                p = start;
                double d = 0.0;
                if (!readNumber(d))
                    return false;
                out = static_cast<std::int64_t>(d);
                return true;
            }
            out = negative ? -value : value;
            return true;
        }
        skipValue();
        out = 0;
        return ok;
    }

    bool readBool(bool &out)
    {
        skipSpace();
        if (end - p >= 4 && std::memcmp(p, "true", 4) == 0)
        {
            p += 4;
            out = true;
            return true;
        }
        if (end - p >= 5 && std::memcmp(p, "false", 5) == 0)
        {
            p += 5;
            out = false;
            return true;
        }
        skipValue();
        out = false;
        return ok;
    }

    // String or null (null -> empty); anything else is skipped.
    bool readStringOrNull(std::string &out)
    {
        const char c = peek();
        if (c == '"')
            return readString(out);
        out.clear();
        skipValue();
        return ok;
    }

    void skipValue()
    {
        // Iterative: nesting depth only counted, never recursed.
        int depth = 0;
        do
        {
            const char c = peek();
            if (!ok || p >= end)
            {
                fail();
                return;
            }
            if (c == '"')
            {
                if (!skipString())
                    return;
            }
            else if (c == '{' || c == '[')
            {
                ++p;
                ++depth;
                continue;
            }
            else if (c == '}' || c == ']')
            {
                ++p;
                --depth;
            }
            else if (c == ',' || c == ':')
            {
                ++p;
                continue;
            }
            else
            {
                // Number, true, false, null.
                const char *start = p;
                while (p < end && *p != ',' && *p != '}' && *p != ']' && *p != ' ' && *p != '\n' && *p != '\r' && *p != '\t')
                    ++p;
                if (p == start)
                {
                    fail();
                    return;
                }
            }
        } while (depth > 0);
    }

private:
    bool fail()
    {
        ok = false;
        return false;
    }

    void skipSpace()
    {
        while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t'))
            ++p;
    }

    bool expect(char c)
    {
        skipSpace();
        if (p < end && *p == c)
        {
            ++p;
            return true;
        }
        return fail();
    }

    bool skipString()
    {
        if (!expect('"'))
            return false;
        while (p < end)
        {
            const char c = *p++;
            if (c == '"')
                return true;
            if (c == '\\' && p < end)
                ++p;
        }
        return fail();
    }

    bool readHex4(unsigned &out)
    {
        if (end - p < 4)
            return false;
        out = 0;
        for (int i = 0; i < 4; ++i)
        {
            const char c = *p++;
            out <<= 4;
            if (c >= '0' && c <= '9') out |= static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') out |= static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') out |= static_cast<unsigned>(c - 'A' + 10);
            else return false;
        }
        return true;
    }

    static void appendUtf8(std::string &out, unsigned cp)
    {
        if (cp < 0x80)
            out.push_back(static_cast<char>(cp));
        else if (cp < 0x800)
        {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else if (cp < 0x10000)
        {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
        else
        {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    const char *p;
    const char *end;
    bool ok;
};

// "1.20.4" -> {1, 20, 4}. Old names ("b1.7.3", "a1.2.6", "rd-132211",
// "inf-20100618") count as 0.x; snapshots "YYwWWa" are modern from 2018 (18w,
// the 1.13 snapshots) on and old before. Returns false for names that are
// not plain releases (snapshots, betas): those never become "newest".
bool splitVersion(const std::string &v, int parts[3])
{
    parts[0] = parts[1] = parts[2] = 0;
    if (v.empty())
        return false;
    // Snapshot: digits, 'w', digits, letter.
    const std::size_t w = v.find('w');
    if (w != std::string::npos && w > 0 && w <= 2)
    {
        const int year = std::atoi(v.substr(0, w).c_str());
        parts[0] = 1;
        parts[1] = year >= 18 ? 13 : 0;
        return false;
    }
    if (v[0] < '0' || v[0] > '9')
        return false; // alpha / beta / classic / infdev: 0.0.0
    int index = 0;
    for (std::size_t i = 0; i < v.size() && index < 3; ++i)
    {
        const char c = v[i];
        if (c >= '0' && c <= '9')
            parts[index] = parts[index] * 10 + (c - '0');
        else if (c == '.')
            ++index;
        else
            return false; // "1.5.2-pre": counted, never "newest"
    }
    return true;
}

bool versionNewer(const std::string &a, const std::string &b)
{
    int pa[3];
    int pb[3];
    if (!splitVersion(a, pa))
        return false;
    if (!splitVersion(b, pb))
        return true;
    for (int i = 0; i < 3; ++i)
        if (pa[i] != pb[i])
            return pa[i] > pb[i];
    return false;
}

bool endsWithZip(const std::string &name)
{
    if (name.size() < 4)
        return false;
    const std::string tail = name.substr(name.size() - 4);
    return (tail[0] == '.') && (tail[1] == 'z' || tail[1] == 'Z') && (tail[2] == 'i' || tail[2] == 'I') &&
        (tail[3] == 'p' || tail[3] == 'P');
}

bool readFile(JsonReader &r, VersionFile &file)
{
    if (!r.beginObject())
        return false;
    std::string key;
    bool first = true;
    while (r.nextKey(key, first))
    {
        if (key == "url")
            r.readStringOrNull(file.url);
        else if (key == "filename")
            r.readStringOrNull(file.fileName);
        else if (key == "size")
            r.readInt64(file.size);
        else if (key == "primary")
            r.readBool(file.primary);
        else if (key == "hashes" && r.peek() == '{')
        {
            r.beginObject();
            std::string hashKey;
            bool hashFirst = true;
            while (r.nextKey(hashKey, hashFirst))
            {
                if (hashKey == "sha1")
                    r.readStringOrNull(file.sha1);
                else
                    r.skipValue();
            }
        }
        else
            r.skipValue();
    }
    for (char &c : file.sha1)
        if (c >= 'A' && c <= 'F')
            c = static_cast<char>(c - 'A' + 'a');
    return r.good();
}
} // namespace

bool parseSearch(const char *data, std::size_t size, SearchPage &out)
{
    out = SearchPage();
    JsonReader r(data, size);
    if (!r.beginObject())
        return false;
    std::string key;
    bool first = true;
    bool sawHits = false;
    while (r.nextKey(key, first))
    {
        if (key == "hits" && r.peek() == '[')
        {
            sawHits = true;
            r.beginArray();
            bool firstHit = true;
            while (r.nextElement(firstHit))
            {
                if (r.peek() != '{')
                {
                    r.skipValue();
                    continue;
                }
                r.beginObject();
                SearchHit hit;
                std::string field;
                bool firstField = true;
                while (r.nextKey(field, firstField))
                {
                    if (field == "project_id") r.readStringOrNull(hit.projectId);
                    else if (field == "slug") r.readStringOrNull(hit.slug);
                    else if (field == "title") r.readStringOrNull(hit.title);
                    else if (field == "author") r.readStringOrNull(hit.author);
                    else if (field == "description") r.readStringOrNull(hit.description);
                    else if (field == "icon_url") r.readStringOrNull(hit.iconUrl);
                    else if (field == "downloads") r.readInt64(hit.downloads);
                    else if (field == "follows") r.readInt64(hit.follows);
                    else r.skipValue();
                }
                if (!r.good())
                    return false;
                if (!hit.projectId.empty())
                {
                    // Long descriptions are cut: two lines on screen at most.
                    if (hit.description.size() > 300)
                        hit.description.resize(300);
                    out.hits.push_back(std::move(hit));
                }
            }
        }
        else if (key == "total_hits")
        {
            std::int64_t v = 0;
            r.readInt64(v);
            out.totalHits = static_cast<int>(v);
        }
        else if (key == "offset")
        {
            std::int64_t v = 0;
            r.readInt64(v);
            out.offset = static_cast<int>(v);
        }
        else
            r.skipValue();
    }
    return r.good() && sawHits;
}

bool parseVersions(const char *data, std::size_t size, const std::string &gameVersion, ProjectVersion &out)
{
    out = ProjectVersion();
    JsonReader r(data, size);
    if (!r.beginArray())
        return false;
    bool found = false;
    bool firstVersion = true;
    while (r.nextElement(firstVersion))
    {
        if (r.peek() != '{')
        {
            r.skipValue();
            continue;
        }
        r.beginObject();
        ProjectVersion v;
        bool listsOurs = false;
        std::vector<VersionFile> files;
        std::string key;
        bool first = true;
        while (r.nextKey(key, first))
        {
            if (key == "name") r.readStringOrNull(v.name);
            else if (key == "version_number") r.readStringOrNull(v.versionNumber);
            else if (key == "date_published") r.readStringOrNull(v.datePublished);
            else if (key == "game_versions" && r.peek() == '[')
            {
                r.beginArray();
                bool firstGv = true;
                std::string gv;
                while (r.nextElement(firstGv))
                {
                    r.readStringOrNull(gv);
                    if (gv.empty())
                        continue;
                    ++v.gameVersionCount;
                    if (gv == gameVersion)
                        listsOurs = true;
                    if (versionNewer(gv, v.newestGameVersion))
                        v.newestGameVersion = gv;
                    int parts[3];
                    splitVersion(gv, parts);
                    if (parts[0] > 1 || (parts[0] == 1 && parts[1] >= 13))
                        v.listsModernVersions = true;
                }
            }
            else if (key == "files" && r.peek() == '[')
            {
                r.beginArray();
                bool firstFile = true;
                while (r.nextElement(firstFile))
                {
                    VersionFile f;
                    if (r.peek() != '{' || !readFile(r, f))
                    {
                        r.skipValue();
                        continue;
                    }
                    files.push_back(std::move(f));
                }
            }
            else
                r.skipValue();
        }
        if (!r.good())
            return false;
        if (!listsOurs || files.empty())
            continue;
        const VersionFile *pick = nullptr;
        for (const VersionFile &f : files)
            if (f.primary && !f.url.empty())
                pick = &f;
        if (pick == nullptr)
            for (const VersionFile &f : files)
                if (endsWithZip(f.fileName) && !f.url.empty())
                {
                    pick = &f;
                    break;
                }
        if (pick == nullptr)
            continue;
        // ISO dates compare as text.
        if (!found || v.datePublished > out.datePublished)
        {
            v.file = *pick;
            out = std::move(v);
            found = true;
        }
    }
    return r.good() && found;
}

namespace
{
void readTextComponent(JsonReader &r, std::string &out, int depth)
{
    const char c = r.peek();
    if (c == '"')
    {
        std::string part;
        r.readString(part);
        out += part;
    }
    else if (c == '{' && depth < 4)
    {
        r.beginObject();
        std::string key;
        bool first = true;
        while (r.nextKey(key, first))
        {
            if (key == "text")
                readTextComponent(r, out, depth + 1);
            else
                r.skipValue();
        }
    }
    else if (c == '[' && depth < 4)
    {
        r.beginArray();
        bool first = true;
        while (r.nextElement(first))
            readTextComponent(r, out, depth + 1);
    }
    else
        r.skipValue();
}
} // namespace

bool parseMcmetaDescription(const char *data, std::size_t size, std::string &out)
{
    out.clear();
    // A UTF-8 BOM is common in hand-written pack.mcmeta files.
    if (size >= 3 && static_cast<unsigned char>(data[0]) == 0xEF && static_cast<unsigned char>(data[1]) == 0xBB &&
        static_cast<unsigned char>(data[2]) == 0xBF)
    {
        data += 3;
        size -= 3;
    }
    JsonReader r(data, size);
    if (!r.beginObject())
        return false;
    std::string key;
    bool first = true;
    while (r.nextKey(key, first))
    {
        if (key == "pack" && r.peek() == '{')
        {
            r.beginObject();
            std::string inner;
            bool innerFirst = true;
            while (r.nextKey(inner, innerFirst))
            {
                if (inner == "description")
                    readTextComponent(r, out, 0);
                else
                    r.skipValue();
            }
        }
        else
            r.skipValue();
    }
    // Strip the section-sign colour codes (U+00A7 + one char).
    std::string clean;
    clean.reserve(out.size());
    for (std::size_t i = 0; i < out.size(); ++i)
    {
        if (static_cast<unsigned char>(out[i]) == 0xC2 && i + 1 < out.size() &&
            static_cast<unsigned char>(out[i + 1]) == 0xA7)
        {
            i += 2;
            continue;
        }
        clean.push_back(out[i]);
    }
    out.swap(clean);
    return !out.empty();
}

std::string urlEncode(const std::string &value)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size() * 3);
    for (unsigned char c : value)
    {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~')
            out.push_back(static_cast<char>(c));
        else
        {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 15]);
        }
    }
    return out;
}
} // namespace TexDl
