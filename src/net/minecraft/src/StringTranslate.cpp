#include "StringTranslate.h"

#include "GameResources.h"
#include "platform/Log.h"
#include <algorithm>
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
            group == "container" || group == "key" || group == "deathScreen" || group == "itemGroup")
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
        {"ON", "options.on"}, {"OFF", "options.off"},
        {"Structures", "itemGroup.buildingBlocks"},
        {"Tools", "itemGroup.tools"},
        {"Food", "itemGroup.food"},
        {"Armour", "itemGroup.combat"},
        {"Armor", "itemGroup.combat"},
        {"Mechanisms", "itemGroup.redstone"},
        {"Transport", "itemGroup.transportation"},
        {"Decoration", "itemGroup.decorations"},
        {"Back", "gui.back"},
        {"Done", "gui.done"},
        {"Cancel", "gui.cancel"}
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

    // 1.5.2 fallback translations for newly ported blocks and items when using
    // unmodified 1.2.5 language files.
    const bool isSpanish = (currentLanguage.rfind("es_", 0) == 0);
    const char *port152Defaults[][3] = {
        {"tile.stairsSandStone.name", "Sandstone Stairs", "Escaleras de arenisca"},
        {"tile.weightedPlate_light.name", "Weighted Pressure Plate (Light)", "Placa de presión ponderada (ligera)"},
        {"tile.weightedPlate_heavy.name", "Weighted Pressure Plate (Heavy)", "Placa de presión ponderada (pesada)"},
        {"tile.daylightDetector.name", "Daylight Sensor", "Sensor de luz diurna"},
        {"tile.tripWireSource.name", "Tripwire Hook", "Gancho de cuerda"},
        {"tile.tripWire.name", "Tripwire", "Cuerda"},
        {"tile.oreEmerald.name", "Emerald Ore", "Mena de esmeralda"},
        {"tile.blockEmerald.name", "Block of Emerald", "Bloque de esmeralda"},
        {"tile.stairsWoodSpruce.name", "Spruce Wood Stairs", "Escaleras de madera de abeto"},
        {"tile.stairsWoodBirch.name", "Birch Wood Stairs", "Escaleras de madera de abedul"},
        {"tile.stairsWoodJungle.name", "Jungle Wood Stairs", "Escaleras de madera de jungla"},
        {"tile.carrots.name", "Carrots", "Zanahorias"},
        {"tile.potatoes.name", "Potatoes", "Patatas"},
        {"tile.blockRedstone.name", "Block of Redstone", "Bloque de redstone"},
        {"tile.comparator.name", "Redstone Comparator", "Comparador de redstone"},
        {"item.comparator.name", "Redstone Comparator", "Comparador de redstone"},
        {"tile.hopper.name", "Hopper", "Tolva"},
        {"tile.dropper.name", "Dropper", "Soltador"},
        {"item.minecartHopper.name", "Minecart with Hopper", "Vagoneta con tolva"},
        {"container.hopper", "Item Hopper", "Tolva"},
        {"container.dropper", "Dropper", "Soltador"},
        {"tile.netherquartz.name", "Nether Quartz Ore", "Mena de cuarzo del Nether"},
        {"tile.quartzBlock.name", "Block of Quartz", "Bloque de cuarzo"},
        {"tile.quartzBlock.default.name", "Block of Quartz", "Bloque de cuarzo"},
        {"tile.quartzBlock.chiseled.name", "Chiseled Quartz Block", "Bloque de cuarzo cincelado"},
        {"tile.quartzBlock.lines.name", "Pillar Quartz Block", "Pilar de cuarzo"},
        {"tile.stairsQuartz.name", "Quartz Stairs", "Escaleras de cuarzo"},
        {"tile.stoneSlab.netherBrick.name", "Nether Brick Slab", "Losa de ladrillos del Nether"},
        {"tile.stoneSlab.quartz.name", "Quartz Slab", "Losa de cuarzo"},
        {"item.diamond.name", "Diamond", "Diamante"},
        {"item.emerald.name", "Emerald", "Esmeralda"},
        {"item.carrots.name", "Carrot", "Zanahoria"},
        {"item.potato.name", "Potato", "Patata"},
        {"item.potatoBaked.name", "Baked Potato", "Patata asada"},
        {"item.potatoPoisonous.name", "Poisonous Potato", "Patata venenosa"},
        {"item.carrotGolden.name", "Golden Carrot", "Zanahoria dorada"},
        {"item.pumpkinPie.name", "Pumpkin Pie", "Tarta de calabaza"},
        {"item.netherbrick.name", "Nether Brick", "Ladrillo del Nether"},
        {"item.netherquartz.name", "Nether Quartz", "Cuarzo del Nether"},
        {"tile.woodSlab.name", "Wooden Slab", "Losa de madera"},
        {"tile.woodSlab.oak.name", "Oak Wood Slab", "Losa de madera de roble"},
        {"tile.woodSlab.spruce.name", "Spruce Wood Slab", "Losa de madera de abeto"},
        {"tile.woodSlab.birch.name", "Birch Wood Slab", "Losa de madera de abedul"},
        {"tile.woodSlab.jungle.name", "Jungle Wood Slab", "Losa de madera de jungla"},
        {"tile.cocoa.name", "Cocoa", "Cacao"},
        {"tile.enderChest.name", "Ender Chest", "Cofre del Fin"},
        {"tile.beacon.name", "Beacon", "Faro"},
        {"container.beacon", "Beacon", "Faro"},
        {"tile.cobbleWall.normal.name", "Cobblestone Wall", "Muro de roca"},
        {"tile.cobbleWall.mossy.name", "Mossy Cobblestone Wall", "Muro de roca musgosa"},
        {"tile.flowerPot.name", "Flower Pot", "Maceta"},
        {"tile.anvil.name", "Anvil", "Yunque"},
        {"tile.anvil.intact.name", "Anvil", "Yunque"},
        {"tile.anvil.slightlyDamaged.name", "Slightly Damaged Anvil", "Yunque ligeramente dañado"},
        {"tile.anvil.veryDamaged.name", "Very Damaged Anvil", "Yunque muy dañado"},
        {"tile.skull.name", "Head", "Cabeza"},
        {"item.skull.skeleton.name", "Skeleton Skull", "Calavera de esqueleto"},
        {"item.skull.wither.name", "Wither Skeleton Skull", "Calavera de esqueleto del Wither"},
        {"item.skull.zombie.name", "Zombie Head", "Cabeza de zombi"},
        {"item.skull.char.name", "Head", "Cabeza"},
        {"item.skull.creeper.name", "Creeper Head", "Cabeza de creeper"},
        {"item.flowerPot.name", "Flower Pot", "Maceta"},
        {"item.netherStar.name", "Nether Star", "Estrella del Nether"},
        {"entity.Bat.name", "Bat", "Murciélago"},
        {"entity.Witch.name", "Witch", "Bruja"},
        {"entity.WitherSkeleton.name", "Wither Skeleton", "Esqueleto del Wither"},
        {"generator.largeBiomes", "Large Biomes", "Biomas grandes"},
        {"selectWorld.bonusItems", "Bonus Chest:", "Cofre extra:"},
        {"item.enchantedBook.name", "Enchanted Book", "Libro encantado"},
        {"item.writingBook.name", "Book and Quill", "Libro y pluma"},
        {"item.writtenBook.name", "Written Book", "Libro escrito"},
        {"item.emptyMap.name", "Empty Map", "Mapa en blanco"},
        {"item.carrotOnAStick.name", "Carrot on a Stick", "Zanahoria en una caña"},
        {"item.frame.name", "Item Frame", "Marco del objeto"},
        {"container.repair", "Repair & Name", "Reparar y Nombrar"},
        {"container.repair.cost", "Enchantment Cost: %1$d", "Coste de Encantamiento: %1$d"},
        {"container.repair.expensive", "Too Expensive!", "¡Demasiado caro!"},
        {"tile.beacon.primary", "Primary Power", "Poder Primario"},
        {"tile.beacon.secondary", "Secondary Power", "Poder Secundario"},
        {"book.editTitle", "Enter Book Title:", "Introduce el Título del Libro:"},
        {"book.signButton", "Sign", "Firmar"},
        {"book.finalizeButton", "Sign and Close", "Firmar y cerrar"},
        {"book.finalizeWarning", "Note! When you sign the book, it will no longer be editable.", "¡Aviso! Una vez firmado, no podrá ser editado."},
        {"book.pageIndicator", "Page %1$s of %2$s", "Página %1$s de %2$s"},
        {"book.byAuthor", "by %1$s", "por %1$s"},
        {"itemGroup.buildingBlocks", "Building Blocks", "Bloques de Construcción"},
        {"itemGroup.decorations", "Decoration Blocks", "Bloques de Decoración"},
        {"itemGroup.redstone", "Redstone", "Redstone"},
        {"itemGroup.transportation", "Transportation", "Transporte"},
        {"itemGroup.misc", "Miscellaneous", "Miscelánea"},
        {"itemGroup.search", "Search Items", "Buscar"},
        {"itemGroup.food", "Foodstuffs", "Comida"},
        {"itemGroup.tools", "Tools", "Herramientas"},
        {"itemGroup.combat", "Combat", "Combate"},
        {"itemGroup.brewing", "Brewing", "Pociones"},
        {"itemGroup.materials", "Materials", "Materiales"},
        {"itemGroup.inventory", "Survival Inventory", "Inventario de modo supervivencia"}
    };
    for (const auto &entry : port152Defaults)
    {
        if (translateTable.find(entry[0]) == translateTable.end())
            translateTable[entry[0]] = isSpanish ? entry[2] : entry[1];
    }

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

// Console trade screen labels (LCE IDS_TOOLTIPS_TRADE / IDS_REQUIRED_ITEMS_FOR_TRADE);
// 1.5.2 has no key for them, so every OptiCraft language is listed here.
static const char *tradeUiText(const std::string &english, const std::string &language)
{
    struct Row { const char *prefix; const char *trade; const char *required; };
    static const Row rows[] = {
        {"af_ZA", "Handel", "Items benodig vir handel"},
        {"ar_SA", "\xd9\x85\xd9\x82\xd8\xa7\xd9\x8a\xd8\xb6\xd8\xa9", "\xd8\xa7\xd9\x84\xd8\xb9\xd9\x86\xd8\xa7\xd8\xb5\xd8\xb1 \xd8\xa7\xd9\x84\xd9\x85\xd8\xb7\xd9\x84\xd9\x88\xd8\xa8\xd8\xa9 \xd9\x84\xd9\x84\xd9\x85\xd9\x82\xd8\xa7\xd9\x8a\xd8\xb6\xd8\xa9"},
        {"bg_BG", "\xd0\xa2\xd1\x8a\xd1\x80\xd0\xb3\xd1\x83\xd0\xb2\xd0\xb0\xd0\xb9", "\xd0\x9d\xd1\x83\xd0\xb6\xd0\xbd\xd0\xb8 \xd0\xbf\xd1\x80\xd0\xb5\xd0\xb4\xd0\xbc\xd0\xb5\xd1\x82\xd0\xb8 \xd0\xb7\xd0\xb0 \xd1\x82\xd1\x8a\xd1\x80\xd0\xb3\xd0\xbe\xd0\xb2\xd0\xb8\xd1\x8f"},
        {"ca_ES", "Intercanviar", "Objectes necessaris per intercanviar"},
        {"cs_CZ", "Obchodovat", "P\xc5\x99" "edm\xc4\x9bty pot\xc5\x99" "ebn\xc3\xa9 k obchodu"},
        {"cy_GB", "Masnachu", "Eitemau sydd eu hangen i fasnachu"},
        {"da_DK", "Byt", "N\xc3\xb8" "dvendige genstande til byttehandel"},
        {"de_DE", "Handeln", "Ben\xc3\xb6tigte Handelsg\xc3\xbcter"},
        {"el_GR", "\xce\x91\xce\xbd\xcf\x84\xce\xb1\xce\xbb\xce\xbb\xce\xb1\xce\xb3\xce\xae", "\xce\x91\xcf\x80\xce\xb1\xce\xb9\xcf\x84\xce\xbf\xcf\x8d\xce\xbc\xce\xb5\xce\xbd\xce\xb1 \xce\xb1\xce\xbd\xcf\x84\xce\xb9\xce\xba\xce\xb5\xce\xaf\xce\xbc\xce\xb5\xce\xbd\xce\xb1 \xce\xb3\xce\xb9\xce\xb1 \xce\xb1\xce\xbd\xcf\x84\xce\xb1\xce\xbb\xce\xbb\xce\xb1\xce\xb3\xce\xae"},
        {"eo_UY", "Inter\xc5\x9d" "an\xc4\x9di", "Necesaj objektoj por inter\xc5\x9d" "an\xc4\x9do"},
        {"es_", "Intercambiar", "Objetos necesarios"},
        {"et_EE", "Kauple", "Kauplemiseks vajalikud esemed"},
        {"eu_ES", "Trukatu", "Trukerako behar diren objektuak"},
        {"fi_FI", "Vaihda", "Vaihtoon tarvittavat esineet"},
        {"fr_", "Commercer", "Objets n\xc3\xa9" "cessaires \xc3\xa0 la transaction"},
        {"gl_ES", "Intercambiar", "Obxectos necesarios para o intercambio"},
        {"he_IL", "\xd7\xa1\xd7\x97\xd7\xa8", "\xd7\xa4\xd7\xa8\xd7\x99\xd7\x98\xd7\x99\xd7\x9d \xd7\xa0\xd7\x93\xd7\xa8\xd7\xa9\xd7\x99\xd7\x9d \xd7\x9c\xd7\xa1\xd7\x97\xd7\xa8"},
        {"ky_KG", "\xd0\xa1\xd0\xbe\xd0\xbe\xd0\xb4\xd0\xb0", "\xd0\xa1\xd0\xbe\xd0\xbe\xd0\xb4\xd0\xb0 \xd2\xaf\xd1\x87\xd2\xaf\xd0\xbd \xd0\xba\xd0\xb5\xd1\x80\xd0\xb5\xd0\xba\xd1\x82\xd2\xaf\xd2\xaf \xd0\xb1\xd1\x83\xd1\x8e\xd0\xbc\xd0\xb4\xd0\xb0\xd1\x80"},
        {"hr_HR", "Trguj", "Predmeti potrebni za trgovinu"},
        {"hu_HU", "Keresked\xc3\xa9s", "Keresked\xc3\xa9shez sz\xc3\xbcks\xc3\xa9ges t\xc3\xa1rgyak"},
        {"is_IS", "Versla", "Hlutir sem \xc3\xbe" "arf til a\xc3\xb0 versla"},
        {"it_IT", "Scambia", "Oggetti richiesti per lo scambio"},
        {"ja_JP", "\xe5\x8f\x96\xe5\xbc\x95", "\xe5\x8f\x96\xe5\xbc\x95\xe3\x81\xab\xe5\xbf\x85\xe8\xa6\x81\xe3\x81\xaa\xe3\x82\xa2\xe3\x82\xa4\xe3\x83\x86\xe3\x83\xa0"},
        {"ka_GE", "\xe1\x83\x95\xe1\x83\x90\xe1\x83\xad\xe1\x83\xa0\xe1\x83\x9d\xe1\x83\x91\xe1\x83\x90", "\xe1\x83\x95\xe1\x83\x90\xe1\x83\xad\xe1\x83\xa0\xe1\x83\x9d\xe1\x83\x91\xe1\x83\x98\xe1\x83\xa1\xe1\x83\x97\xe1\x83\x95\xe1\x83\x98\xe1\x83\xa1 \xe1\x83\xa1\xe1\x83\x90\xe1\x83\xad\xe1\x83\x98\xe1\x83\xa0\xe1\x83\x9d \xe1\x83\x9c\xe1\x83\x98\xe1\x83\x95\xe1\x83\x97\xe1\x83\x94\xe1\x83\x91\xe1\x83\x98"},
        {"ko_KR", "\xea\xb1\xb0\xeb\x9e\x98", "\xea\xb1\xb0\xeb\x9e\x98\xec\x97\x90 \xed\x95\x84\xec\x9a\x94\xed\x95\x9c \xec\x95\x84\xec\x9d\xb4\xed\x85\x9c"},
        {"lt_LT", "Prekiauti", "Prekybai reikalingi daiktai"},
        {"lv_LV", "Tirgoties", "Tirdzniec\xc4\xab" "bai nepiecie\xc5\xa1" "amie priek\xc5\xa1meti"},
        {"mi_NZ", "Hokohoko", "Ng\xc4\x81 taonga e hiahiatia ana hei hokohoko"},
        {"mt_MT", "Innegozja", "O\xc4\xa1\xc4\xa1" "etti me\xc4\xa7tie\xc4\xa1" "a g\xc4\xa7" "an-negozju"},
        {"nl_NL", "Handelen", "Benodigde voorwerpen voor ruil"},
        {"nn_NO", "Byt", "Gjenstandar som trengst for byte"},
        {"nb_NO", "Bytt", "Gjenstander som trengs for byttehandel"},
        {"pl_PL", "Handluj", "Przedmioty potrzebne do wymiany"},
        {"pt_BR", "Negociar", "Itens necess\xc3\xa1rios para negociar"},
        {"pt_PT", "Trocar", "Necess\xc3\xa1rio para Troca"},
        {"ro_RO", "Negociaz\xc4\x83", "Obiecte necesare pentru schimb"},
        {"ru_RU", "\xd0\xa2\xd0\xbe\xd1\x80\xd0\xb3\xd0\xbe\xd0\xb2\xd0\xb0\xd1\x82\xd1\x8c", "\xd0\x9f\xd1\x80\xd0\xb5\xd0\xb4\xd0\xbc\xd0\xb5\xd1\x82\xd1\x8b \xd0\xb4\xd0\xbb\xd1\x8f \xd0\xbe\xd0\xb1\xd0\xbc\xd0\xb5\xd0\xbd\xd0\xb0"},
        {"sk_SK", "Obchodova\xc5\xa5", "Predmety potrebn\xc3\xa9 na obchod"},
        {"sl_SI", "Trguj", "Predmeti, potrebni za trgovanje"},
        {"sr_SP", "\xd0\xa2\xd1\x80\xd0\xb3\xd1\x83\xd1\x98", "\xd0\x9f\xd1\x80\xd0\xb5\xd0\xb4\xd0\xbc\xd0\xb5\xd1\x82\xd0\xb8 \xd0\xbf\xd0\xbe\xd1\x82\xd1\x80\xd0\xb5\xd0\xb1\xd0\xbd\xd0\xb8 \xd0\xb7\xd0\xb0 \xd1\x82\xd1\x80\xd0\xb3\xd0\xbe\xd0\xb2\xd0\xb8\xd0\xbd\xd1\x83"},
        {"sv_SE", "Byt", "F\xc3\xb6rem\xc3\xa5l som kr\xc3\xa4vs f\xc3\xb6r byteshandel"},
        {"th_TH", "\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\x9b\xe0\xb8\xa5\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb8\xa2\xe0\xb8\x99", "\xe0\xb9\x84\xe0\xb8\xad\xe0\xb9\x80\xe0\xb8\x97\xe0\xb9\x87\xe0\xb8\xa1\xe0\xb8\x97\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb8\x95\xe0\xb9\x89\xe0\xb8\xad\xe0\xb8\x87\xe0\xb9\x83\xe0\xb8\x8a\xe0\xb9\x89\xe0\xb9\x83\xe0\xb8\x99\xe0\xb8\x81\xe0\xb8\xb2\xe0\xb8\xa3\xe0\xb9\x81\xe0\xb8\xa5\xe0\xb8\x81\xe0\xb9\x80\xe0\xb8\x9b\xe0\xb8\xa5\xe0\xb8\xb5\xe0\xb9\x88\xe0\xb8\xa2\xe0\xb8\x99"},
        {"tr_TR", "Takas", "Takas i\xc3\xa7in gereken e\xc5\x9fyalar"},
        {"uk_UA", "\xd0\xa2\xd0\xbe\xd1\x80\xd0\xb3\xd1\x83\xd0\xb2\xd0\xb0\xd1\x82\xd0\xb8", "\xd0\x9f\xd1\x80\xd0\xb5\xd0\xb4\xd0\xbc\xd0\xb5\xd1\x82\xd0\xb8 \xd0\xb4\xd0\xbb\xd1\x8f \xd0\xbe\xd0\xb1\xd0\xbc\xd1\x96\xd0\xbd\xd1\x83"},
        {"vi_VN", "Giao d\xe1\xbb\x8b" "ch", "V\xe1\xba\xadt ph\xe1\xba\xa9m c\xe1\xba\xa7n \xc4\x91\xe1\xbb\x83 giao d\xe1\xbb\x8b" "ch"},
        {"zh_CN", "\xe4\xba\xa4\xe6\x98\x93", "\xe4\xba\xa4\xe6\x98\x93\xe6\x89\x80\xe9\x9c\x80\xe7\x89\xa9\xe5\x93\x81"},
        {"zh_TW", "\xe4\xba\xa4\xe6\x98\x93", "\xe4\xba\xa4\xe6\x98\x93\xe6\x89\x80\xe9\x9c\x80\xe9\x81\x93\xe5\x85\xb7"},
    };
    const bool trade = english == "Trade";
    if (!trade && english != "Required items for trade")
        return nullptr;
    for (const Row &row : rows)
        if (language.rfind(row.prefix, 0) == 0)
            return trade ? row.trade : row.required;
    return nullptr;
}

std::string StringTranslate::translateUi(const std::string &english)
{
    auto extra = translateTable.find("ui." + english);
    if (extra != translateTable.end() && !extra->second.empty())
        return extra->second;
    auto key = englishUiKeys.find(english);
    if (key != englishUiKeys.end())
        return translateKey(key->second);
    if (const char *trade = tradeUiText(english, currentLanguage))
        return trade;
    // Source labels may have an ellipsis or a colon that the vanilla key lacks.
    const std::string suffix = english.size() >= 3 && english.compare(english.size() - 3, 3, "...") == 0
        ? "..." : (english.size() >= 2 && english.compare(english.size() - 2, 2, ": ") == 0 ? ": " : "");
    if (!suffix.empty())
        return translateUi(english.substr(0, english.size() - suffix.size())) + suffix;
    if (currentLanguage.rfind("es_", 0) == 0)
    {
        if (english == "Split Screen") return "Pantalla dividida";
        if (english == "Horizontal") return "Horizontal";
        if (english == "Vertical") return "Vertical";
        if (english == "Toggle") return "Alternar";
        if (english == "Delete") return "Eliminar";
        if (english == "Structures") return "Estructuras";
        if (english == "Tools") return "Herramientas";
        if (english == "Food") return "Comida";
        if (english == "Armour" || english == "Armor") return "Armadura";
        if (english == "Mechanisms") return "Mecanismos";
        if (english == "Transport") return "Transporte";
        if (english == "Decoration") return "Decoraci\xc3\xb3n";
        if (english == "Craft") return "Fabricar";
        if (english == "Back") return "Atr\xc3\xa1s";
        if (english == "Group") return "Grupo";
        if (english == "Ingredients") return "Ingredientes";
        if (english == "Inventory") return "Inventario";
        if (english == "Nothing to craft here") return "No hay nada que fabricar aqu\xc3\xad";
        if (english == "Category") return "Categor\xc3\xad" "a";
        if (english == "Navigate") return "Navegar";
        if (english == "Play Tutorial") return "Jugar tutorial";
        if (english == "Tutorial World") return "Mundo tutorial";
        if (english == "Could not load Tutorial World") return "No se pudo cargar el mundo tutorial";
        if (english == "No Games Found") return "No se encontraron partidas";
        if (english == "Reset to Defaults") return "Restablecer valores";
        if (english == "Press a button...") return "Pulsa un bot\xc3\xb3n...";
        if (english == "Press a key...") return "Pulsa una tecla...";
        if (english == "Select a command to edit.") return "Selecciona un comando para editar.";
        if (english == "Previous") return "Anterior";
        if (english == "Next") return "Siguiente";
        if (english == "Done") return "Listo";
        if (english == "Cancel") return "Cancelar";
        if (english == "Fullbright") return "Brillo m\xc3\xa1ximo";
        if (english == "Show Coordinates") return "Mostrar coordenadas";
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
        // Java's String.format: the arguments are already strings, so %d works like %s.
        std::size_t pos = std::min(result.find("%s", searchFrom), result.find("%d", searchFrom));
        if (pos == std::string::npos)
            break;
        result.replace(pos, 2, args[sequentialArg]);
        searchFrom = pos + args[sequentialArg].length();
        ++sequentialArg;
    }

    for (std::size_t i = 0; i < args.size(); ++i)
    {
        for (const char *suffix : {"$s", "$d"})
        {
            const std::string token = "%" + std::to_string(i + 1) + suffix;
            std::size_t pos = 0;
            while ((pos = result.find(token, pos)) != std::string::npos)
            {
                result.replace(pos, token.length(), args[i]);
                pos += args[i].length();
            }
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

std::string StringTranslate::translateKeyFormat(const std::string &s, const std::string &arg1, const std::string &arg2)
{
    return translateKeyFormat(s, std::vector<std::string>{arg1, arg2});
}

bool StringTranslate::containsTranslateKey(const std::string &s) const
{
    if (translateTable.find(s) != translateTable.end())
        return true;
    if (s == "key.crafting")
        return true;
    return false;
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

    bool hasDiamondKey = false;
    std::string fileEmeraldValue;

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
        {
            if (!ui)
            {
                if (key == "item.diamond.name")
                    hasDiamondKey = true;
                else if (key == "item.emerald.name")
                    fileEmeraldValue = value;
            }
            translateTable[(ui ? "ui." : "") + key] = value;
        }
    }

    // In 1.2.5 .lang files, Item::diamond used the legacy key "item.emerald.name".
    // When loading a 1.2.5 language file (which lacks "item.diamond.name"), map
    // its "item.emerald.name" value to "item.diamond.name" and clear "item.emerald.name"
    // so 1.5.2 Item::emerald receives the proper emerald name instead of "Diamond".
    if (!ui && !hasDiamondKey && !fileEmeraldValue.empty())
    {
        translateTable["item.diamond.name"] = fileEmeraldValue;
        translateTable.erase("item.emerald.name");
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
