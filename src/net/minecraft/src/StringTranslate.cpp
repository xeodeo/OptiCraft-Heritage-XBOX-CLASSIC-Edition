#include "StringTranslate.h"

#include "GameResources.h"
#include "platform/Log.h"
#include <memory>

namespace
{
std::unique_ptr<std::istream> openLanguageResource(const std::string &path)
{
    return GameResources::open(path);
}

bool hasCodepointAtLeast256(const std::string &text)
{
    for (std::size_t i = 0; i < text.size();)
    {
        unsigned char c = (unsigned char)text[i];
        unsigned int codepoint = 0;
        std::size_t length = 1;
        if (c < 0x80)
        {
            codepoint = c;
        }
        else if ((c & 0xe0) == 0xc0 && i + 1 < text.size())
        {
            codepoint = ((c & 0x1f) << 6) | ((unsigned char)text[i + 1] & 0x3f);
            length = 2;
        }
        else if ((c & 0xf0) == 0xe0 && i + 2 < text.size())
        {
            codepoint = ((c & 0x0f) << 12) | (((unsigned char)text[i + 1] & 0x3f) << 6)
                      | ((unsigned char)text[i + 2] & 0x3f);
            length = 3;
        }
        else if ((c & 0xf8) == 0xf0 && i + 3 < text.size())
        {
            codepoint = ((c & 0x07) << 18) | (((unsigned char)text[i + 1] & 0x3f) << 12)
                      | (((unsigned char)text[i + 2] & 0x3f) << 6)
                      | ((unsigned char)text[i + 3] & 0x3f);
            length = 4;
        }
        else
        {
            codepoint = 0x100;
        }
        if (codepoint >= 0x100)
            return true;
        i += length;
    }
    return false;
}

#ifdef PS2_PLATFORM
// Whole-file version of hasCodepointAtLeast256: true only if every value in
// the file (key=value lines; keys are always plain-ASCII identifiers, so
// only values are worth checking) stays inside codepoints 0..255. A missing
// or unreadable file returns false -- exclude rather than guess.
bool languageFileIsLatin1Only(const std::string &path)
{
    std::unique_ptr<std::istream> owned = openLanguageResource(path);
    std::istream *input = owned.get();
    if (input == nullptr || !(*input))
        return false;

    std::string line;
    while (std::getline(*input, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        if (hasCodepointAtLeast256(line))
            return false;
    }
    return true;
}
#endif
}

StringTranslate *StringTranslate::instance = nullptr;

StringTranslate::StringTranslate()
    : currentLanguage()
    , unicode(false)
{
    loadLanguageList();
    setLanguage("en_US");
}

StringTranslate *StringTranslate::getInstance()
{
    if (instance == nullptr)
        instance = new StringTranslate();
    return instance;
}

const std::map<std::string, std::string> &StringTranslate::getLanguageList() const
{
    return languageList;
}

void StringTranslate::filterToLatinLanguagesOnPs2()
{
#ifdef PS2_PLATFORM
    if (latinFiltered)
        return;
    latinFiltered = true;

    for (auto it = languageList.begin(); it != languageList.end(); )
    {
        // en_US is always ASCII by construction (it's the mandatory fallback
        // loaded first in setLanguage()); skip the redundant file read.
        if (it->first == "en_US" || languageFileIsLatin1Only("/lang/" + it->first + ".lang"))
            ++it;
        else
            it = languageList.erase(it);
    }

    if (languageList.find("en_US") == languageList.end())
        languageList["en_US"] = "English (US)";
#endif
}

bool StringTranslate::isLatin1SafeLanguageOnPs2(const std::string &language)
{
#ifdef PS2_PLATFORM
    if (language == "en_US")
        return true;
    return languageFileIsLatin1Only("/lang/" + language + ".lang");
#else
    (void)language;
    return true;
#endif
}

void StringTranslate::loadLanguageList()
{
    languageList.clear();
    std::unique_ptr<std::istream> owned = openLanguageResource("/lang/languages.txt");
    std::istream *input = owned.get();
    if (input != nullptr)
    {
        std::string line;
        while (std::getline(*input, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            std::size_t equals = line.find('=');
            if (equals == std::string::npos)
                continue;
            std::string key = trim(line.substr(0, equals));
            std::string value = trim(line.substr(equals + 1));
            if (!key.empty() && !value.empty())
                languageList[key] = value;
        }
    }
    if (languageList.empty())
        languageList["en_US"] = "English (US)";
}

void StringTranslate::setLanguage(const std::string &language)
{
    if (language == currentLanguage && !translateTable.empty())
        return;

    translateTable.clear();
    englishUiKeys.clear();
    loadLanguageFile("/lang/en_US.lang");
    for (const auto &entry : translateTable)
    {
        const auto dot = entry.first.find('.');
        const std::string group = entry.first.substr(0, dot);
        if (group == "gui" || group == "menu" || group == "options" || group == "controls" ||
            group == "selectWorld" || group == "createWorld" || group == "gameMode" ||
            group == "multiplayer" || group == "disconnect" || group == "connect" ||
            group == "container" || group == "key" || group == "deathScreen")
            englishUiKeys.emplace(entry.second, entry.first);
    }
    const char *aliases[][2] = {
        {"Play Game", "menu.singleplayer"}, {"Help & Options", "menu.options"},
        {"Start Game", "selectWorld.title"}, {"Create New World", "selectWorld.create"},
        {"Resume Game", "menu.returnToGame"}, {"Save & Quit", "menu.returnToMenu"},
        {"Video", "options.video"}, {"Controls", "options.controls"},
        {"Language", "options.language"}, {"Fancy Graphics", "options.graphics"},
        {"Smooth Lighting", "options.ao"}, {"View Bobbing", "options.viewBobbing"},
        {"Render Clouds", "options.renderClouds"}, {"Render Distance", "options.renderDistance"},
        {"FOV", "options.fov"}, {"Sensitivity", "options.sensitivity"},
        {"Invert Mouse", "options.invertMouse"}, {"Attack", "key.attack"},
        {"Use", "key.use"}, {"Jump", "key.jump"}, {"Sneak", "key.sneak"},
        {"Drop", "key.drop"}, {"Inventory", "key.inventory"},
        {"ON", "options.on"}, {"OFF", "options.off"}
    };
    for (const auto &alias : aliases)
        if (translateTable.find(alias[1]) != translateTable.end())
            englishUiKeys[alias[0]] = alias[1];
    englishUiKeys["Crafting"] = "key.crafting";
    currentLanguage = "en_US";
    if (language != "en_US" && loadLanguageFile("/lang/" + language + ".lang"))
        currentLanguage = language;
    // Supplemental files use English labels as keys and do not alter the
    // existing resource files. Missing files/entries keep the English text.
    loadLanguageFile("/lang/ui/" + currentLanguage + ".lang", true);
    updateUnicodeFlag();
}

const std::string &StringTranslate::getCurrentLanguage() const
{
    return currentLanguage;
}

bool StringTranslate::isUnicode() const
{
    return unicode;
}

bool StringTranslate::isBidirectional(const std::string &language)
{
    return language == "ar_SA" || language == "he_IL";
}

std::string StringTranslate::translateUi(const std::string &english)
{
    auto extra = translateTable.find("ui." + english);
    if (extra != translateTable.end() && !extra->second.empty())
        return extra->second;
    auto key = englishUiKeys.find(english);
    if (key != englishUiKeys.end())
        return translateKey(key->second);
    // Source labels may have an ellipsis or a colon that the vanilla key lacks.
    const std::string suffix = english.size() >= 3 && english.compare(english.size() - 3, 3, "...") == 0
        ? "..." : (english.size() >= 2 && english.compare(english.size() - 2, 2, ": ") == 0 ? ": " : "");
    if (!suffix.empty())
        return translateUi(english.substr(0, english.size() - suffix.size())) + suffix;
    if (currentLanguage.rfind("es_", 0) == 0)
    {
        if (english == "Split Screen") return "Pantalla dividida";
        if (english == "Texture Packs") return "Packs de texturas";
        if (english == "Horizontal") return "Horizontal";
        if (english == "Vertical") return "Vertical";
        if (english == "Toggle") return "Alternar";
        if (english == "Delete") return "Eliminar";
        if (english == "Sneak: Hold") return "Agacharse: mantener";
        if (english == "Sneak: Toggle") return "Agacharse: alternar";
        if (english == "Previous") return "Anterior";
        if (english == "Next") return "Siguiente";
        if (english == "Done") return "Listo";
        if (english == "Cancel") return "Cancelar";
        if (english == "Fullbright") return "Brillo m\xc3\xa1ximo";
        if (english == "Show Coordinates") return "Mostrar coordenadas";
        if (english == "On-Screen Hints") return "Pistas en pantalla";
        if (english == "Safe Area") return "Zona segura";
        if (english == "30 FPS Limit") return "L\xc3\xadmite de 30 FPS";
        if (english == "Deflicker Filter") return "Filtro antiparpadeo";
        if (english == "Fog") return "Niebla";
        if (english == "Dolby Digital") return "Dolby Digital";
        if (english == "Advanced") return "Avanzado";
        if (english == "Details") return "Detalles";
        if (english == "Animations") return "Animaciones";
        if (english == "Quality") return "Calidad";
        if (english == "Performance") return "Rendimiento";
        if (english == "Other") return "Otros";
        if (english == "All ON") return "Todo S\xc3\x8d";
        if (english == "All OFF") return "Todo NO";
        if (english == "Default") return "Por defecto";
        if (english == "Fast") return "R\xc3\xa1pido";
        if (english == "Fancy") return "Detallado";
        if (english == "Dynamic") return "Din\xc3\xa1mico";
        if (english == "Day Only") return "Solo d\xc3\xad" "a";
        if (english == "Night Only") return "Solo noche";
        if (english == "Clouds") return "Nubes";
        if (english == "Cloud Height") return "Altura de nubes";
        if (english == "Trees") return "\xc3\x81rboles";
        if (english == "Grass") return "Hierba";
        if (english == "Water") return "Agua";
        if (english == "Rain & Snow") return "Lluvia y nieve";
        if (english == "Sky") return "Cielo";
        if (english == "Stars") return "Estrellas";
        if (english == "Sun & Moon") return "Sol y luna";
        if (english == "Depth Fog") return "Niebla de profundidad";
        if (english == "Fog Start") return "Inicio de niebla";
        if (english == "Water Animated") return "Agua animada";
        if (english == "Lava Animated") return "Lava animada";
        if (english == "Fire Animated") return "Fuego animado";
        if (english == "Portal Animated") return "Portal animado";
        if (english == "Redstone Animated") return "Redstone animada";
        if (english == "Explosion Animated") return "Explosi\xc3\xb3n animada";
        if (english == "Flame Animated") return "Llama animada";
        if (english == "Smoke Animated") return "Humo animado";
        if (english == "Void Particles") return "Part\xc3\xad" "culas del vac\xc3\xado";
        if (english == "Water Particles") return "Part\xc3\xad" "culas de agua";
        if (english == "Rain Splash") return "Salpicaduras de lluvia";
        if (english == "Portal Particles") return "Part\xc3\xad" "culas de portal";
        if (english == "Dripping Water/Lava") return "Goteo de agua/lava";
        if (english == "Terrain Animated") return "Terreno animado";
        if (english == "Items Animated") return "Objetos animados";
        if (english == "Textures Animated") return "Texturas animadas";
        if (english == "Clear Water") return "Agua clara";
        if (english == "Random Mobs") return "Criaturas aleatorias";
        if (english == "Better Grass") return "Hierba mejorada";
        if (english == "Better Snow") return "Nieve mejorada";
        if (english == "Opaque Ice") return "Hielo opaco";
        if (english == "Pocket Optimizations") return "Optimizaciones Pocket";
        if (english == "Custom Fonts") return "Fuentes personalizadas";
        if (english == "Custom Colors") return "Colores personalizados";
        if (english == "Swamp Colors") return "Colores de pantano";
        if (english == "Smooth Biomes") return "Biomas suavizados";
        if (english == "Connected Textures") return "Texturas conectadas";
        if (english == "Natural Textures") return "Texturas naturales";
        if (english == "Mipmap Level") return "Nivel de mipmap";
        if (english == "Mipmap Type") return "Tipo de mipmap";
        if (english == "Antialiasing") return "Antialiasing";
        if (english == "Anisotropic Filtering") return "Filtro anisotr\xc3\xb3pico";
        if (english == "Smooth FPS") return "FPS estables";
        if (english == "Smooth Input") return "Entrada fluida";
        if (english == "Load Far") return "Cargar lejos";
        if (english == "Preloaded Chunks") return "Chunks precargados";
        if (english == "Chunk Updates") return "Actualizaciones de chunks";
        if (english == "Dynamic Updates") return "Actualizaciones din\xc3\xa1micas";
        if (english == "Far View") return "Vista lejana";
        if (english == "Fast Debug Info") return "Depuraci\xc3\xb3n r\xc3\xa1pida";
        if (english == "Debug Profiler") return "Perfilador";
        if (english == "Weather") return "Clima";
        if (english == "Time") return "Tiempo";
        if (english == "Autosave") return "Autoguardado";
    }
    return english;
}

std::string StringTranslate::translateKey(const std::string &s)
{
    auto it = translateTable.find(s);
    if (it != translateTable.end())
        return it->second;
    if (s == "key.crafting")
    {
        if (currentLanguage.rfind("es_", 0) == 0)
            return "Fabricar";
        return "Crafting";
    }
    return s;
}

std::string StringTranslate::translateKeyFormat(const std::string &s, const std::vector<std::string> &args)
{
    std::string result = translateKey(s);
    std::size_t sequentialArg = 0;
    std::size_t searchFrom = 0;
    while (sequentialArg < args.size())
    {
        std::size_t pos = result.find("%s", searchFrom);
        if (pos == std::string::npos)
            break;
        result.replace(pos, 2, args[sequentialArg]);
        searchFrom = pos + args[sequentialArg].length();
        ++sequentialArg;
    }

    for (std::size_t i = 0; i < args.size(); ++i)
    {
        std::string token = "%" + std::to_string(i + 1) + "$s";
        std::size_t pos = 0;
        while ((pos = result.find(token, pos)) != std::string::npos)
        {
            result.replace(pos, token.length(), args[i]);
            pos += args[i].length();
        }
    }
    return result;
}

std::string StringTranslate::translateKeyFormat(const std::string &s, const std::string &arg)
{
    return translateKeyFormat(s, std::vector<std::string>{arg});
}

std::string StringTranslate::translateKeyFormat(const std::string &s, const char *arg)
{
    return translateKeyFormat(s, std::vector<std::string>{arg != nullptr ? arg : ""});
}

std::string StringTranslate::translateNamedKey(const std::string &s)
{
    auto it = translateTable.find(s + ".name");
    return it != translateTable.end() ? it->second : "";
}

bool StringTranslate::loadLanguageFile(const std::string &path, bool ui)
{
    std::unique_ptr<std::istream> owned = openLanguageResource(path);
    std::istream *input = owned.get();
    if (input == nullptr || !(*input))
    {
#ifdef PS2_PLATFORM
        MC_LOG_DEBUG("ps2", "language missing: %s\n", path.c_str());
#endif
        return false;
    }

    std::string line;
    while (std::getline(*input, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        std::size_t equals = line.find('=');
        if (equals == std::string::npos)
            continue;
        const std::string key = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1));
        // Empty overrides must not erase the English fallback.
        if (!key.empty() && !value.empty())
            translateTable[(ui ? "ui." : "") + key] = value;
    }
    return true;
}

std::string StringTranslate::trim(const std::string &s)
{
    std::size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    std::size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

void StringTranslate::updateUnicodeFlag()
{
    unicode = false;
    for (const auto &entry : translateTable)
    {
        if (hasCodepointAtLeast256(entry.second))
        {
            unicode = true;
            break;
        }
    }
}
