#include "GuiChocopedia.h"
#include "EntityChocobo.h"

#include "net/minecraft/src/Minecraft.h"
#include "net/minecraft/src/GameSettings.h"
#include "net/minecraft/src/World.h"
#include "net/minecraft/src/EntityPlayer.h"
#include "net/minecraft/src/EntityPlayerSP.h"
#include "net/minecraft/src/GuiButton.h"
#include "net/minecraft/src/GuiTextField.h"
#include "net/minecraft/src/FontRenderer.h"
#include "net/minecraft/src/RenderEngine.h"
#include "net/minecraft/src/SoundManager.h"
#include "net/minecraft/src/StringTranslate.h"
#include "net/minecraft/src/ControlIcon.h"
#include "net/minecraft/src/legacy/LegacyMenuHints.h"
#include "net/minecraft/src/legacy/LegacyMenuWidgets.h"
#include "platform/PlatformConfig.h"
#include "platform/Input.h"
#include "platform/RenderAPI.h"

#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
#include "net/minecraft/src/VirtualKeyboard.h"
#endif

#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
#include "pc/lwjgl/Keyboard.h"
#endif

#include <algorithm>
#include <cstdio>

namespace
{
#define C_GOLD "\xC2\xA7" "6"
#define C_GREEN "\xC2\xA7" "a"
#define C_BLUE "\xC2\xA7" "9"
#define C_GRAY "\xC2\xA7" "7"
#define C_DGRAY "\xC2\xA7" "8"
#define C_BLACK "\xC2\xA7" "0"

// ChocopediaPages (4.1.5), page 0 is the title page. The original wraps its text by
// hand for a 116 px wide book (hyphens included); here it is wrapped at run time.
const char *const kPagesEn[] = {
    "",
    "Table of Contents\n\nI. Chocobos\nII. In the wild\nIII. Gysahl Greens\n .1 Gysahl Mutations\n .2 Gysahl Pickles\n"
    "IV. Domestication\n .1 Breeding Chocobos\n .2 Chocobo Colours\nV. The Nether\nVI. Chocobo Pen\nVII. Chocobo Cake",
    "I. Chocobos\n\nThese large, herbivorous birds are largely unable to fly, but their powerful legs allow them to run "
    "at high speeds. Their size and impressive speed make them a popular choice when looking for a mount.",
    "Although largely too small to allow real flight, the Chocobo's wings enable the birds to glide and avoid fall "
    "damage to itself and its rider up to a certain height.\n\nDomesticated Chocobos can be deployed as mounts and "
    "transport animals.",
    "There are even reports of desperate adventurers having to rely on their Chocobos as a source of food.",
    "II. In the wild\n\nChocobos, in their yellow variety, are found wild in most parts of the world. Flocks of mostly "
    "three, sometimes five animals roam free in the wild. However they are very scarce and an interested "
    "ornithologist would have to search",
    "wide areas to find a flock.",
    "III. Gysahl Greens\n\nThe Chocobo's favorite food are Gysahl Greens, these thick red root vegetables can be found "
    "growing widespread in all fertile areas. These plants can be crafted into seeds. The seeds then can be used to "
    "domestically grow",
    "Gysahl Greens on tilled and moisturised land.",
    "III.1 Gysahl Mutations\n\nWhen domestically growing Gysahl Greens, there is a chance of the root mutating into a "
    "Loverly or even Golden Gysahl. These special roots can be fed to two Chocobos of opposite gender, to initialise "
    "the mating process.",
    "III.2 Gysahl Pickles\n\nGysahl Greens and sugar can be crafted into raw Gysahl pickles, which can be cooked in a "
    "furnace to receive Gysahl pickles. These treats are not only tasty, but also very nourishing.",
    "IV. Domestication\n\nChocobos can be tamed by feeding them Gysahl Greens, the birds are very fond of the root "
    "vegetable and will sometimes form a bond to whomever is feeding them their favorite food item. If tamed, the "
    "owner can fit either a chocobo",
    "saddle or pack bags on a Chocobo. Fitted with a saddle the Chocobo can be additionally equipped with saddle bags. "
    "(Pack bags and saddle bags can be accessed from the chocobo menu: sneak and use.)",
    "IV.1 Chocobo Breeding\n\nIf fed with Loverly or Golden Gysahls, Chocobos of different gender can be bred to "
    "produce an offspring. These infant Chocobos are called Chicobos and eventually grow into a Chocobo. Besides "
    "increasing the number",
    "of Chocobos in a domesticated flock, breeding is the only way to obtain the different varieties of Chocobos "
    "available. There is a slight chance of mutation, which can be increased with the use of Golden Gysahls. The "
    "following combinations of parents can cause",
    "their offspring to mutate into these subspecies:\n\n 1. " C_GOLD "Yellow" C_BLACK " + " C_GOLD "Yellow" C_BLACK
    "\n = " C_GREEN "Green" C_BLACK " or " C_BLUE "Blue" C_BLACK "\n\n 2. " C_GREEN "Green" C_BLACK " + " C_BLUE "Blue"
    C_BLACK "\n = " C_GRAY "White" C_BLACK "\n\n 3. " C_GOLD "Yellow" C_BLACK " + " C_GRAY "White" C_BLACK "\n = Black",
    "4. Black + " C_GRAY "White" C_BLACK "\n = " C_GOLD "Gold" C_BLACK "\n\nTo enable the offspring to mutate into a "
    "golden Chocobo, the use of Golden Gysahls is mandatory.",
    "IV.2 Chocobo Colours\n\n" C_GOLD "Yellow Chocobos" C_BLACK " are the average variety and have no clear abilities, "
    "they can be used however as fast riding and transport animals.\n\n" C_GREEN "Green Chocobos" C_BLACK " have the "
    "ability to climb, which means they do",
    "not have the need to jump when ascending a block level. They are slightly faster than their yellow cousins.\n\n"
    C_BLUE "Blue Chocobos" C_BLACK " have the ability to travel fast on water. Although all Chocobos have the ability "
    "to swim, the blue ones can do it very fast,",
    "thus are the first choice of transportation when traveling by water route.\n\n" C_GRAY "White Chocobos" C_BLACK
    " have the combined abilities to climb like the green Chocobos and to travel fast on water like the blue. In "
    "addition they are slightly faster than the blue",
    "or green varieties.\n\nBlack Chocobos have the abilities to jump very high, climb and travel fast on water. Also "
    "they are faster than the white ones.\n\n" C_GOLD "Golden Chocobos" C_BLACK " are the fastest variety and are not "
    "only able to glide like the other",
    "subspecies, but can fully fly.\n\nAll Chocobo subspecies can glide and avoid fall damage up to a certain height. "
    "Golden Chocobos don't take fall damage at all.",
    "V. The Nether\n\nThe Nether is populated by a special breed of Chocobo. The Purple Chocobos roam the hostile "
    "tunnels and caves of the nether in search of the scarce available food. This makes them even harder to find than "
    "their yellow",
    "cousins. Living in the nether had the purple Chocobos develop the ability to swim in lava and fly.",
    "VI. Chocobo Pen\n\nAn area covered with straw and equipped with a water filled cauldron will count as a Chocobo "
    "pen. If a hurt Chocobo will stand on the straw near the cauldron it will automatically heal over time.",
    "VII. Chocobo Cake\n\nChocobo Cakes can be used to have Chicobos instantly grow up into Chocobos.\n\nRecipe:\n"
    "Take an egg, mix it with two units of sugar, two buckets of milk and two bundles of grain. Finally garnish it "
    "with two Gysahl Greens.\nEnjoy.",
};

const char *const kPagesEs[] = {
    "",
    "Índice\n\nI. Chocobos\nII. En libertad\nIII. Verduras de Gysahl\n .1 Mutaciones de Gysahl\n .2 Encurtidos de Gysahl\n"
    "IV. Domesticación\n .1 Cría de chocobos\n .2 Colores de chocobo\nV. El Inframundo\nVI. Corral de chocobos\n"
    "VII. Pastel de chocobo",
    "I. Chocobos\n\nEstas grandes aves herbívoras apenas pueden volar, pero sus potentes patas les permiten correr a "
    "gran velocidad. Su tamaño y su rapidez las convierten en una montura muy apreciada.",
    "Aunque sus alas son demasiado pequeñas para volar de verdad, les permiten planear y evitar el daño por caída, a "
    "ellas y a su jinete, hasta cierta altura.\n\nLos chocobos domesticados sirven de montura y de animal de carga.",
    "Hay incluso relatos de aventureros desesperados que tuvieron que recurrir a sus chocobos como fuente de comida.",
    "II. En libertad\n\nLos chocobos, en su variedad amarilla, viven en libertad en casi todo el mundo. Bandadas de "
    "tres animales, a veces cinco, vagan libres. Pero son muy escasos y un ornitólogo interesado tendría que recorrer",
    "grandes extensiones para encontrar una bandada.",
    "III. Verduras de Gysahl\n\nLa comida favorita del chocobo es la verdura de Gysahl, una raíz roja y gruesa que "
    "crece en todas las tierras fértiles. Con ella se fabrican semillas, que luego sirven para cultivar",
    "verduras de Gysahl en tierra arada y húmeda.",
    "III.1 Mutaciones de Gysahl\n\nAl cultivar verduras de Gysahl, la raíz puede mutar en una Gysahl amorosa o incluso "
    "dorada. Estas raíces especiales se dan a dos chocobos de distinto sexo para que se apareen.",
    "III.2 Encurtidos de Gysahl\n\nCon verdura de Gysahl y azúcar se fabrica encurtido de Gysahl crudo, que se cocina "
    "en el horno para obtener encurtido de Gysahl. Este manjar no solo es sabroso: también alimenta mucho.",
    "IV. Domesticación\n\nLos chocobos se doman dándoles verdura de Gysahl; les encanta y a veces crean un vínculo con "
    "quien se la da. Una vez domado, el dueño puede ponerle al chocobo",
    "una silla o una mochila. Con la silla puesta, el chocobo puede llevar además unas alforjas. (Las alforjas y la "
    "mochila se abren desde el menú del chocobo: agáchate y úsalo.)",
    "IV.1 Cría de chocobos\n\nSi se les da Gysahl amorosa o dorada, dos chocobos de distinto sexo tendrán una cría. "
    "Estas crías se llaman chicobos y con el tiempo se convierten en chocobos. Además de aumentar el número",
    "de chocobos del corral, la cría es la única forma de conseguir las distintas variedades de chocobo. Hay una "
    "pequeña probabilidad de mutación, que aumenta con la Gysahl dorada. Estas parejas de padres pueden hacer",
    "que la cría mute en estas variedades:\n\n 1. " C_GOLD "Amarillo" C_BLACK " + " C_GOLD "Amarillo" C_BLACK
    "\n = " C_GREEN "Verde" C_BLACK " o " C_BLUE "Azul" C_BLACK "\n\n 2. " C_GREEN "Verde" C_BLACK " + " C_BLUE "Azul"
    C_BLACK "\n = " C_GRAY "Blanco" C_BLACK "\n\n 3. " C_GOLD "Amarillo" C_BLACK " + " C_GRAY "Blanco" C_BLACK
    "\n = Negro",
    "4. Negro + " C_GRAY "Blanco" C_BLACK "\n = " C_GOLD "Dorado" C_BLACK "\n\nPara que la cría pueda mutar en un "
    "chocobo dorado es imprescindible usar Gysahl dorada.",
    "IV.2 Colores de chocobo\n\nLos " C_GOLD "chocobos amarillos" C_BLACK " son la variedad común y no tienen "
    "habilidades especiales, pero son buenas monturas rápidas y animales de carga.\n\nLos " C_GREEN "chocobos verdes"
    C_BLACK " saben escalar, es decir,",
    "no necesitan saltar para subir un bloque. Son algo más rápidos que sus primos amarillos.\n\nLos " C_BLUE
    "chocobos azules" C_BLACK " viajan rápido por el agua. Aunque todos los chocobos saben nadar, los azules lo hacen "
    "muy deprisa,",
    "por eso son la primera elección para viajar por agua.\n\nLos " C_GRAY "chocobos blancos" C_BLACK " suman las "
    "habilidades de escalar como los verdes y de ir rápido por el agua como los azules. Además son algo más rápidos "
    "que los azules",
    "o los verdes.\n\nLos chocobos negros saltan muy alto, escalan y viajan rápido por el agua. También son más rápidos "
    "que los blancos.\n\nLos " C_GOLD "chocobos dorados" C_BLACK " son la variedad más rápida y no solo planean como "
    "las demás",
    "variedades: pueden volar de verdad.\n\nTodas las variedades de chocobo planean y evitan el daño por caída hasta "
    "cierta altura. Los chocobos dorados nunca sufren daño por caída.",
    "V. El Inframundo\n\nEl Inframundo está habitado por una raza especial de chocobo. Los chocobos morados recorren "
    "sus túneles y cuevas hostiles en busca de la escasa comida. Por eso son aún más difíciles de encontrar que sus "
    "primos",
    "amarillos. Vivir en el Inframundo ha hecho que los chocobos morados sepan nadar en lava y volar.",
    "VI. Corral de chocobos\n\nUna zona cubierta de paja y con un caldero lleno de agua cuenta como corral de chocobos. "
    "Si un chocobo herido se queda sobre la paja cerca del caldero, se curará poco a poco.",
    "VII. Pastel de chocobo\n\nEl pastel de chocobo hace que un chicobo crezca al instante hasta ser un chocobo.\n\n"
    "Receta:\nToma un huevo, mézclalo con dos de azúcar, dos cubos de leche y dos de trigo. Por último, adórnalo con "
    "dos verduras de Gysahl.\n¡Que aproveche!",
};

constexpr int_t kPageCount = static_cast<int_t>(sizeof(kPagesEn) / sizeof(kPagesEn[0]));
static_assert(sizeof(kPagesEn) == sizeof(kPagesEs), "Chocopedia pages: same count in both languages");

// Word wrap that keeps blank lines (paragraphs) and colour codes.
std::vector<std::string> wrapText(FontRenderer *font, const std::string &text, int_t maxWidth)
{
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= text.size())
    {
        std::size_t nl = text.find('\n', start);
        if (nl == std::string::npos)
            nl = text.size();
        const std::string para = text.substr(start, nl - start);
        if (para.empty())
        {
            out.push_back("");
        }
        else
        {
            std::string line;
            std::size_t p = 0;
            while (p < para.size())
            {
                std::size_t sp = para.find(' ', p);
                if (sp == std::string::npos)
                    sp = para.size();
                const std::string word = para.substr(p, sp - p);
                const std::string candidate = line.empty() ? word : line + " " + word;
                if (!line.empty() && font->getStringWidth(candidate) > maxWidth)
                {
                    out.push_back(line);
                    line = word;
                }
                else
                {
                    line = candidate;
                }
                p = sp + 1;
            }
            out.push_back(line);
        }
        start = nl + 1;
    }
    return out;
}

// Active colour code (from the last section sign) to carry into the next wrapped line.
std::string carryColour(const std::string &line, const std::string &current)
{
    std::string colour = current;
    for (std::size_t i = 0; i + 2 < line.size(); ++i)
    {
        if (static_cast<unsigned char>(line[i]) == 0xC2 && static_cast<unsigned char>(line[i + 1]) == 0xA7)
            colour = line.substr(i, 3);
    }
    return colour;
}

int_t drawWrapped(FontRenderer *font, const std::string &text, int_t x, int_t y, int_t maxWidth, int_t maxY,
                  int_t color, int_t lineH)
{
    std::string colour;
    for (const std::string &line : wrapText(font, text, maxWidth))
    {
        if (y + 8 > maxY)
            break;
        if (!line.empty())
            font->drawString(colour + line, x, y, color);
        colour = carryColour(line, colour);
        y += line.empty() ? lineH / 2 + 1 : lineH;
    }
    return y;
}

Minecraft *mcInstance()
{
    return Minecraft::getMinecraft();
}

int playerIndexOf(Minecraft *mc, EntityPlayer *player)
{
    return (mc != nullptr && player != nullptr && player == mc->thePlayer2 && mc->isSplitScreenActive()) ? 1 : 0;
}
} // namespace

// ---------------------------------------------------------------------------

void GuiChocopedia::open(EntityPlayer *player, EntityChocobo *chocobo)
{
    Minecraft *mc = mcInstance();
    if (mc == nullptr || player == nullptr)
        return;
    const int idx = playerIndexOf(mc, player);
    GuiChocopedia *screen = new GuiChocopedia(chocobo, player, idx);
    if (idx == 1)
        mc->displayPlayerScreen(1, screen);
    else
        mc->displayGuiScreen(screen);
}

bool GuiChocopedia::isScreenOpenFor(EntityPlayer *player)
{
    Minecraft *mc = mcInstance();
    if (mc == nullptr || player == nullptr)
        return false;
    return mc->getPlayerScreen(playerIndexOf(mc, player)) != nullptr;
}

GuiChocopedia::GuiChocopedia(EntityChocobo *chocobo, EntityPlayer *player, int playerIndex)
    : m_chocobo(chocobo)
    , m_player(player)
    , m_playerIndex(playerIndex)
    , m_mode(chocobo != nullptr ? Mode::Info : Mode::Book)
    , m_page(0)
    , m_selected(0)
    , m_renaming(false)
    , m_openLatch(true)
    , m_javaRowsBuilt(-1)
    , m_javaModeBuilt(Mode::Book)
    , m_nameField(nullptr)
{
}

GuiChocopedia::~GuiChocopedia()
{
    delete m_nameField;
    m_nameField = nullptr;
}

bool GuiChocopedia::isLegacyMode() const
{
    return mc != nullptr && mc->gameSettings != nullptr && mc->gameSettings->legacyUI;
}

bool GuiChocopedia::spanish() const
{
    StringTranslate *tr = StringTranslate::getInstance();
    return tr != nullptr && tr->getCurrentLanguage().rfind("es_", 0) == 0;
}

bool GuiChocopedia::usesSpecializedMenuNavigation() const
{
    if (m_renaming)
        return false;
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    return true;
#else
    return isLegacyMode();
#endif
}

int_t GuiChocopedia::pageCount() const
{
    return kPageCount;
}

void GuiChocopedia::initGui()
{
    m_openLatch = true;
    m_renaming = false;
    delete m_nameField;
    const int_t fw = std::min<int_t>(180, width - 40);
    m_nameField = new GuiTextField(this, fontRenderer, (width - fw) / 2, height / 2 - 44, fw, 18, "");
    m_nameField->setMaxStringLength(20);
    m_nameField->setFocused(false);
    m_javaRowsBuilt = -1;
    rebuildJavaButtons();
}

void GuiChocopedia::onGuiClosed()
{
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
    if (m_nameField != nullptr)
        m_nameField->setFocused(false);
    GuiScreen::onGuiClosed();
}

void GuiChocopedia::updateScreen()
{
    GuiScreen::updateScreen();
    if (m_mode != Mode::Book && (m_chocobo == nullptr || m_chocobo->isDead))
    {
        closeScreen();
        return;
    }
    if (m_renaming && m_nameField != nullptr)
        m_nameField->updateCursorCounter();
}

std::vector<std::string> GuiChocopedia::infoLines() const
{
    std::vector<std::string> lines;
    if (m_chocobo == nullptr)
        return lines;
    const bool es = spanish();
    char buf[128];
    lines.push_back(std::string(es ? "Dueño: " : "Owner: ") + m_chocobo->getOwnerName());
    std::snprintf(buf, sizeof(buf), "%s%d/%d%s", es ? "Salud: " : "Health: ", static_cast<int>(m_chocobo->getHealth()),
                  static_cast<int>(m_chocobo->getMaxHealth()),
                  m_chocobo->isHungry() ? (es ? " (hambriento)" : " (hungry)") : "");
    lines.push_back(buf);
    const bool canBreed = m_chocobo->getGrowingAge() <= 0 && !m_chocobo->isChild();
    std::string gender = m_chocobo->isMale() ? (es ? "Macho" : "Male") : (es ? "Hembra" : "Female");
    gender += canBreed ? (es ? " (puede criar)" : " (can breed)") : (es ? " (no puede criar)" : " (cannot breed)");
    lines.push_back(gender);
    lines.push_back(std::string(EntityChocobo::getColorName(m_chocobo->getColor(), es)) + " - " +
                    EntityChocobo::getColorAbility(m_chocobo->getColor(), es));
    return lines;
}

void GuiChocopedia::buildRows(std::vector<Row> &rows) const
{
    rows.clear();
    const bool es = spanish();
    if (m_mode == Mode::Book)
    {
        rows.push_back({ACT_PREV_PAGE, "<", m_page > 0});
        rows.push_back({ACT_NEXT_PAGE, ">", m_page < pageCount() - 1});
        rows.push_back({ACT_DONE, es ? "Hecho" : "Done", true});
        return;
    }
    if (m_chocobo == nullptr)
        return;
    const bool owner = m_chocobo->isOwner(m_player);
    if (m_mode == Mode::Info)
    {
        const char *follow = "";
        switch (m_chocobo->getAiMode())
        {
        case ChocoboAiMode::Follow: follow = es ? "Seguir" : "Follow"; break;
        case ChocoboAiMode::Wander: follow = es ? "Deambular" : "Wander"; break;
        case ChocoboAiMode::Stay:   follow = es ? "Quedarse" : "Stay"; break;
        }
        rows.push_back({ACT_RENAME, es ? "Cambiar nombre" : "Rename", owner});
        rows.push_back({ACT_HIDENAME, m_chocobo->isHidename() ? (es ? "Nombre oculto" : "Name Hidden")
                                                                : (es ? "Nombre visible" : "Name Shown"), owner});
        rows.push_back({ACT_FOLLOW, std::string(es ? "Modo: " : "Mode: ") + follow, owner});
        rows.push_back({ACT_CHANGE_OWNER, es ? "Cambiar dueño" : "Change owner", owner});
        if (m_chocobo->riddenByEntity == nullptr && (m_chocobo->isSaddled() || m_chocobo->hasPackbags()))
            rows.push_back({ACT_DROP_GEAR, es ? "Quitar equipo" : "Drop Gear", owner});
        rows.push_back({ACT_CONFIRM, es ? "Confirmar" : "Confirm", true});
    }
    else if (m_mode == Mode::SelectOwner)
    {
        for (std::size_t i = 0; i < m_players.size(); ++i)
            rows.push_back({ACT_PLAYER_BASE + static_cast<int_t>(i), m_players[i], true});
        rows.push_back({ACT_CANCEL, es ? "Cancelar" : "Cancel", true});
    }
    else
    {
        rows.push_back({ACT_ACCEPT, es ? "Aceptar" : "Accept", true});
        rows.push_back({ACT_CANCEL, es ? "Cancelar" : "Cancel", true});
    }
}

// Java look: GuiButtons for the rows (the mouse works on PC; on consoles the pad
// selection is shown with the keyboard-selected highlight).
void GuiChocopedia::rebuildJavaButtons()
{
    if (isLegacyMode())
    {
        if (!controlList.empty())
            clearControlList();
        m_javaRowsBuilt = -1;
        return;
    }
    std::vector<Row> rows;
    buildRows(rows);
    if (m_javaRowsBuilt == static_cast<int_t>(rows.size()) && m_javaModeBuilt == m_mode &&
        controlList.size() == rows.size())
    {
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            controlList[i]->displayString = rows[i].label;
            controlList[i]->enabled = rows[i].enabled;
        }
        return;
    }
    clearControlList();
    const bool isSplit = mc != nullptr && mc->isSplitScreenActive();
    const int_t hintY = legacyHintRowY(height, isSplit);
    if (m_mode == Mode::Book)
    {
        const bool book = height >= 230;
        const int_t bottom = book ? 196 : hintY - 34;
        controlList.push_back(new GuiButton(ACT_PREV_PAGE, width / 2 - 100, bottom, 20, 20, "<"));
        controlList.push_back(new GuiButton(ACT_NEXT_PAGE, width / 2 + 80, bottom, 20, 20, ">"));
        controlList.push_back(new GuiButton(ACT_DONE, width / 2 - 70, bottom, 140, 20, rows[2].label));
    }
    else
    {
        const int_t count = static_cast<int_t>(rows.size());
        const int_t top = std::min<int_t>(44, height / 6 + 10);
        const int_t step = std::max<int_t>(21, std::min<int_t>(24, (hintY - 6 - top) / std::max<int_t>(1, count)));
        const bool infoMode = m_mode == Mode::Info;
        const int_t bw = infoMode ? 100 : 200;
        const int_t bx = infoMode ? width / 2 - 110 : width / 2 - 100;
        for (int_t i = 0; i < count; ++i)
            controlList.push_back(new GuiButton(rows[i].action, bx, top + i * step, bw, 20, rows[i].label));
    }
    for (std::size_t i = 0; i < rows.size() && i < controlList.size(); ++i)
        controlList[i]->enabled = rows[i].enabled;
    m_javaRowsBuilt = static_cast<int_t>(rows.size());
    m_javaModeBuilt = m_mode;
}

void GuiChocopedia::setMode(Mode mode)
{
    m_mode = mode;
    m_selected = 0;
    m_javaRowsBuilt = -1;
    if (mode == Mode::SelectOwner)
    {
        // GuiSelectNewOwner: the players in the world (both players in split screen).
        m_players.clear();
        if (m_chocobo != nullptr && m_chocobo->worldObj != nullptr)
        {
            for (EntityPlayer *p : m_chocobo->worldObj->playerEntities)
            {
                if (p != nullptr && p != m_player && !p->username.empty())
                    m_players.push_back(p->username);
            }
        }
    }
    rebuildJavaButtons();
}

void GuiChocopedia::turnPage(int_t delta)
{
    const int_t next = std::clamp<int_t>(m_page + delta, 0, pageCount() - 1);
    if (next != m_page)
    {
        m_page = next;
        playClick("random.click");
    }
}

void GuiChocopedia::playClick(const char *sound)
{
    if (mc != nullptr && mc->sndManager != nullptr)
        mc->sndManager->playSoundFX(sound, 1.0f, 1.0f);
}

void GuiChocopedia::activate(int_t action)
{
    std::vector<Row> rows;
    buildRows(rows);
    bool enabled = false;
    for (const Row &r : rows)
        if (r.action == action)
            enabled = r.enabled;
    if (!enabled)
        return;

    switch (action)
    {
    case ACT_PREV_PAGE: turnPage(-1); return;
    case ACT_NEXT_PAGE: turnPage(1); return;
    case ACT_DONE:
    case ACT_CONFIRM:
        closeScreen();
        return;
    case ACT_RENAME:
        beginRename();
        return;
    case ACT_HIDENAME:
        m_chocobo->setHidename(!m_chocobo->isHidename());
        playClick("random.click");
        return;
    case ACT_FOLLOW:
        m_chocobo->toggleFollowWanderStay();
        playClick("random.click");
        return;
    case ACT_CHANGE_OWNER:
        playClick("random.click");
        setMode(Mode::SelectOwner);
        return;
    case ACT_DROP_GEAR:
        m_chocobo->dropGear();
        closeScreen();
        return;
    case ACT_ACCEPT:
        // GuiChangeOwner: only to a player who is still in the world.
        if (m_chocobo->worldObj != nullptr)
        {
            for (EntityPlayer *p : m_chocobo->worldObj->playerEntities)
            {
                if (p != nullptr && std::string(p->username) == m_newOwner)
                {
                    m_chocobo->changeOwner(m_newOwner);
                    playClick("random.orb");
                    break;
                }
            }
        }
        setMode(Mode::Info);
        return;
    case ACT_CANCEL:
        playClick("random.click");
        setMode(m_mode == Mode::ConfirmOwner ? Mode::SelectOwner : Mode::Info);
        return;
    default:
        break;
    }
    if (action >= ACT_PLAYER_BASE && action - ACT_PLAYER_BASE < static_cast<int_t>(m_players.size()))
    {
        m_newOwner = m_players[static_cast<std::size_t>(action - ACT_PLAYER_BASE)];
        playClick("random.click");
        setMode(Mode::ConfirmOwner);
    }
}

void GuiChocopedia::actionPerformed(GuiButton *button)
{
    if (button != nullptr && button->enabled)
        activate(button->id);
}

void GuiChocopedia::beginRename()
{
    if (m_nameField == nullptr || m_chocobo == nullptr)
        return;
    m_renaming = true;
    m_nameField->setText(m_chocobo->getCustomName());
    m_nameField->setFocused(true);
    playClick("random.click");
}

void GuiChocopedia::commitRename()
{
    if (m_chocobo != nullptr && m_nameField != nullptr)
    {
        // GuiNameChocobo.setName: the name changes, shown or hidden stays as it was.
        const bool hidden = m_chocobo->isHidename();
        m_chocobo->setCustomName(m_nameField->getText());
        m_chocobo->setHidename(hidden);
        playClick("random.orb");
    }
    cancelRename();
}

void GuiChocopedia::cancelRename()
{
    m_renaming = false;
    m_openLatch = true;
    if (m_nameField != nullptr)
        m_nameField->setFocused(false);
#if defined(PS2_PLATFORM) || defined(WII_PLATFORM) || defined(XBOX_PLATFORM)
    VirtualKeyboard::instance().releaseFocus();
#endif
}

void GuiChocopedia::closeScreen()
{
    cancelRename();
    if (mc == nullptr)
        return;
    playClick("random.back");
    if (mc->isSplitScreenActive() && m_playerIndex == 1)
    {
        mc->closePlayerScreen(1);
    }
    else
    {
        mc->displayGuiScreen(nullptr);
        mc->setIngameFocus();
    }
}

void GuiChocopedia::handleSpecializedMenuInput()
{
    if (m_renaming)
        return;
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    const PlatformTextInputSnapshot pad = platformTextInputSnapshot(platformMenuPad());
    if (!pad.connected)
        return;
    if (m_openLatch)
    {
        if ((pad.held & (PLATFORM_TEXT_TYPE | PLATFORM_TEXT_CLOSE | PLATFORM_TEXT_BACK | PLATFORM_TEXT_SHIFT)) == 0)
            m_openLatch = false;
        return;
    }

    if (pad.pressed & PLATFORM_TEXT_CLOSE)
    {
        if (m_mode == Mode::SelectOwner || m_mode == Mode::ConfirmOwner)
        {
            playClick("random.back");
            setMode(m_mode == Mode::ConfirmOwner ? Mode::SelectOwner : Mode::Info);
        }
        else
        {
            closeScreen();
        }
        return;
    }

    if (m_mode == Mode::Book)
    {
        // D-pad / LB-RB turn the pages, A turns forward and closes on the last one.
        if (pad.pressed & (PLATFORM_TEXT_LEFT | PLATFORM_TEXT_PREV_PAGE))
            turnPage(-1);
        else if (pad.pressed & (PLATFORM_TEXT_RIGHT | PLATFORM_TEXT_NEXT_PAGE))
            turnPage(1);
        else if (pad.pressed & PLATFORM_TEXT_TYPE)
        {
            if (m_page < pageCount() - 1)
                turnPage(1);
            else
                closeScreen();
        }
        return;
    }

    std::vector<Row> rows;
    buildRows(rows);
    const int_t count = static_cast<int_t>(rows.size());
    if (count == 0)
        return;
    m_selected = std::clamp<int_t>(m_selected, 0, count - 1);
    if (pad.pressed & PLATFORM_TEXT_UP)
    {
        m_selected = (m_selected + count - 1) % count;
        playClick("random.focus");
    }
    else if (pad.pressed & PLATFORM_TEXT_DOWN)
    {
        m_selected = (m_selected + 1) % count;
        playClick("random.focus");
    }
    else if (pad.pressed & PLATFORM_TEXT_TYPE)
    {
        activate(rows[static_cast<std::size_t>(m_selected)].action);
    }
#endif
}

void GuiChocopedia::keyTyped(char_t c, int_t key)
{
    if (m_renaming && m_nameField != nullptr)
    {
        if (key == 1)
        {
            cancelRename();
            return;
        }
        if (key == 28 || c == '\r' || c == '\n')
        {
            commitRename();
            return;
        }
        m_nameField->textboxKeyTyped(c, key);
        return;
    }
    if (key == 1)
    {
        if (m_mode == Mode::SelectOwner || m_mode == Mode::ConfirmOwner)
            setMode(m_mode == Mode::ConfirmOwner ? Mode::SelectOwner : Mode::Info);
        else
            closeScreen();
        return;
    }
#if !PLATFORM_PS2 && !PLATFORM_WII && !PLATFORM_XBOX
    if (m_mode == Mode::Book)
    {
        if (key == lwjgl::Keyboard::KEY_LEFT) { turnPage(-1); return; }
        if (key == lwjgl::Keyboard::KEY_RIGHT) { turnPage(1); return; }
    }
#endif
    GuiScreen::keyTyped(c, key);
}

void GuiChocopedia::drawHints(int_t hintY)
{
#if PLATFORM_XBOX
    if (m_renaming)
        return;
    const bool es = spanish();
    if (m_mode == Mode::Book)
    {
        const std::string btns[] = {"D-Pad", "A", "B"};
        const std::string acts[] = {es ? "Pasar página" : "Turn page", es ? "Siguiente" : "Next", es ? "Cerrar" : "Close"};
        drawControlHintRow(mc, width, hintY, btns, acts, 3);
    }
    else
    {
        const std::string btns[] = {"D-Pad", "A", "B"};
        const std::string acts[] = {es ? "Navegar" : "Navigate", es ? "Elegir" : "Select",
                                    m_mode == Mode::Info ? (es ? "Cerrar" : "Close") : (es ? "Atrás" : "Back")};
        drawControlHintRow(mc, width, hintY, btns, acts, 3);
    }
#else
    (void)hintY;
#endif
}

void GuiChocopedia::drawRenameBox(bool legacy)
{
    if (!m_renaming || m_nameField == nullptr)
        return;
    const bool es = spanish();
    const int_t boxW = std::min<int_t>(224, width - 8);
    const int_t boxX = (width - boxW) / 2;
    const int_t by = std::max<int_t>(4, height / 2 - 70);
    const std::string hdr = es ? "Nombre del chocobo (Start: guardar)" : "Chocobo name (Start: save)";
    if (legacy)
    {
        legacyDrawMenuPanel(boxX, by, boxW, 54);
        legacyDrawPanelTitle(fontRenderer, legacyFitText(fontRenderer, hdr, boxW - 12), width / 2, by + 8);
    }
    else
    {
        drawRect(boxX - 2, by - 2, boxX + boxW + 2, by + 56, static_cast<int_t>(0xFFAAAAAAu));
        drawRect(boxX, by, boxX + boxW, by + 54, static_cast<int_t>(0xF0181818u));
        drawCenteredString(fontRenderer, hdr, width / 2, by + 6, 0xFFFFAA);
    }
    m_nameField->drawTextBox();
}

void GuiChocopedia::drawScreen(int_t mouseX, int_t mouseY, float_t partialTick)
{
    if (m_mode != Mode::Book && m_chocobo == nullptr)
        return;
    if (isLegacyMode())
    {
        if (!controlList.empty())
            rebuildJavaButtons();
        drawLegacy(partialTick);
        return;
    }
    rebuildJavaButtons();
    drawJava(mouseX, mouseY, partialTick);
}

void GuiChocopedia::drawJava(int_t mouseX, int_t mouseY, float_t partialTick)
{
    const bool es = spanish();
    const bool isSplit = mc != nullptr && mc->isSplitScreenActive();
    const int_t hintY = legacyHintRowY(height, isSplit);
#if PLATFORM_PS2 || PLATFORM_WII || PLATFORM_XBOX
    for (std::size_t i = 0; i < controlList.size(); ++i)
        controlList[i]->setKeyboardSelected(m_mode != Mode::Book && static_cast<int_t>(i) == m_selected);
#endif

    if (m_mode == Mode::Book)
    {
        drawDefaultBackground();
        const bool haveBook = height >= 230 && mc != nullptr && mc->renderEngine != nullptr &&
                              mc->renderEngine->hasResource("/gui/book.png");
        const int_t bookW = 192;
        int_t x0;
        int_t textX, textY, textW, textBottom;
        if (haveBook)
        {
            // GuiChocopedia's book: /gui/book.png, text 116 px wide at +36,34.
            renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/book.png"));
            x0 = (width - bookW) / 2;
            drawTexturedModalRect(x0, 2, 0, 0, bookW, 192);
            textX = x0 + 36;
            textY = 34;
            textW = 116;
            textBottom = 170;
        }
        else
        {
            const int_t pw = std::min<int_t>(240, width - 16);
            x0 = (width - pw) / 2;
            const int_t top = 6;
            const int_t bottom = hintY - 40;
            drawRect(x0 - 1, top - 1, x0 + pw + 1, bottom + 1, static_cast<int_t>(0xFF5A3A1Au));
            drawRect(x0, top, x0 + pw, bottom, static_cast<int_t>(0xFFF3E5C0u));
            textX = x0 + 10;
            textY = top + 18;
            textW = pw - 20;
            textBottom = bottom - 4;
        }
        if (m_page == 0)
        {
            const std::string title = C_GOLD "The Chocopedia";
            const std::string author = C_DGRAY "by Clienthax";
            fontRenderer->drawString(title, textX + (textW - fontRenderer->getStringWidth(title)) / 2, textY + 16, 0);
            fontRenderer->drawString(author, textX + (textW - fontRenderer->getStringWidth(author)) / 2, textY + 26, 0);
        }
        else
        {
            char ind[48];
            std::snprintf(ind, sizeof(ind), es ? "Página %d de %d" : "Page %d of %d", static_cast<int>(m_page),
                          static_cast<int>(pageCount() - 1));
            fontRenderer->drawString(ind, textX + textW - fontRenderer->getStringWidth(ind), textY - 14, 0);
            drawWrapped(fontRenderer, es ? kPagesEs[m_page] : kPagesEn[m_page], textX, textY, textW, textBottom, 0, 9);
        }
        GuiScreen::drawScreen(mouseX, mouseY, partialTick);
        drawHints(hintY);
        return;
    }

    drawGradientRect(0, 0, width, height, static_cast<int_t>(0xC0080808u), static_cast<int_t>(0xE0141414u));
    if (m_mode == Mode::Info)
    {
        // GuiChocopedia (chocobo page): name on top, buttons on the left, data on the right.
        drawCenteredString(fontRenderer, m_chocobo->getDisplayName(es), width / 2, std::min<int_t>(20, height / 12), 0xFFFFFF);
        const int_t top = std::min<int_t>(44, height / 6 + 10);
        const std::vector<std::string> lines = infoLines();
        const int_t step = isSplit ? 12 : 24;
        int_t y = top + 6;
        for (const std::string &line : lines)
        {
            drawString(fontRenderer, legacyFitText(fontRenderer, line, width / 2 - 4), width / 2 + 0, y, 0xA0A0A0);
            y += step;
        }
    }
    else if (m_mode == Mode::SelectOwner)
    {
        drawCenteredString(fontRenderer, es ? "Dar tu chocobo a otro jugador:" : "Give your Chocobo to new owner:",
                           width / 2, std::min<int_t>(20, height / 12), 0xFFFFFF);
        if (m_players.empty())
            drawCenteredString(fontRenderer, es ? "(no hay más jugadores en el mundo)" : "(no other players in the world)",
                               width / 2, std::min<int_t>(32, height / 8), 0xA0A0A0);
    }
    else
    {
        const std::string q = (es ? "¿Dar tu " : "Give your ") +
                              std::string(m_chocobo->isChild() ? "Chicobo" : "Chocobo") + (es ? " a " : " to ") +
                              m_newOwner + "?";
        drawCenteredString(fontRenderer, q, width / 2, std::min<int_t>(20, height / 12), 0xFFFFFF);
    }
    GuiScreen::drawScreen(mouseX, mouseY, partialTick);
    drawRenameBox(false);
    drawHints(hintY);
}

void GuiChocopedia::drawLegacy(float_t partialTick)
{
    const bool es = spanish();
    legacyDrawMenuBackdrop(mc, width, height, partialTick);
    const bool compact = height < 180;

    if (m_mode == Mode::Book)
    {
        const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, 280, 1, compact ? 96 : 150, 0);
        // One tall panel for the page; the single "row" slot is not drawn.
        const int_t panelBottom = l.rowY(0) + l.rowH;
        legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, panelBottom - l.panelY + 6);
        legacyDrawPanelTitle(fontRenderer, "Chocopedia", l.panelX + l.panelW / 2, l.titleY);
        const int_t textX = l.contentX + 4;
        const int_t textW = l.contentW - 8;
        const int_t textY = l.titleY + 14;
        const int_t bottom = panelBottom;
        if (m_page == 0)
        {
            const std::string title = C_GOLD "The Chocopedia";
            const std::string author = C_DGRAY "by Clienthax";
            fontRenderer->drawString(title, l.panelX + (l.panelW - fontRenderer->getStringWidth(title)) / 2, textY + 18, 0);
            fontRenderer->drawString(author, l.panelX + (l.panelW - fontRenderer->getStringWidth(author)) / 2, textY + 30, 0);
            const std::string tip = es ? "Usa el libro sobre un chocobo domado para ver sus datos."
                                       : "Use the book on a tamed chocobo to see its page.";
            legacyDrawPanelTextWrapped(fontRenderer, tip, l.panelX + l.panelW / 2, textY + 52, textW,
                                       static_cast<int_t>(0xff606060u), 3);
        }
        else
        {
            drawWrapped(fontRenderer, es ? kPagesEs[m_page] : kPagesEn[m_page], textX, textY, textW, bottom, 0x303030, 9);
        }
        char ind[48];
        std::snprintf(ind, sizeof(ind), "%d / %d", static_cast<int>(m_page + 1), static_cast<int>(pageCount()));
        legacyDrawPanelText(fontRenderer, ind, l.panelX + l.panelW - 10 - fontRenderer->getStringWidth(ind), l.titleY,
                            static_cast<int_t>(0xff606060u));
        drawHints(l.hintY);
        return;
    }

    std::vector<Row> rows;
    buildRows(rows);
    const int_t count = static_cast<int_t>(rows.size());
    m_selected = std::clamp<int_t>(m_selected, 0, std::max<int_t>(0, count - 1));

    std::vector<std::string> header;
    std::string title;
    if (m_mode == Mode::Info)
    {
        title = m_chocobo->getDisplayName(es);
        header = infoLines();
        if (compact && header.size() > 2)
        {
            // Split screen: owner + health on one line, gender + colour on the other.
            header = {header[0] + "  " + header[1], header[2]};
        }
    }
    else if (m_mode == Mode::SelectOwner)
    {
        title = es ? "Cambiar dueño" : "Change owner";
        header.push_back(m_players.empty() ? (es ? "No hay más jugadores en el mundo." : "No other players in the world.")
                                           : (es ? "Dar tu chocobo a:" : "Give your Chocobo to:"));
    }
    else
    {
        title = es ? "Cambiar dueño" : "Change owner";
        header.push_back((es ? "¿Dar tu " : "Give your ") + std::string(m_chocobo->isChild() ? "Chicobo" : "Chocobo") +
                         (es ? " a " : " to ") + m_newOwner + "?");
    }

    const int_t lineH = compact ? 9 : 10;
    const int_t headerH = (compact ? 14 : 18) + static_cast<int_t>(header.size()) * lineH;
    const LegacyMenuLayout l = legacyMenuLayout(mc, width, height, 260, count, headerH, 0);
    legacyDrawMenuPanel(l.panelX, l.panelY, l.panelW, l.panelH);
    legacyDrawPanelTitle(fontRenderer, title, l.panelX + l.panelW / 2, l.titleY);
    int_t y = l.titleY + (compact ? 11 : 13);
    for (const std::string &line : header)
    {
        const std::string fit = legacyFitText(fontRenderer, line, l.contentW);
        legacyDrawPanelText(fontRenderer, fit, l.panelX + (l.panelW - fontRenderer->getStringWidth(fit)) / 2, y,
                            static_cast<int_t>(0xff606060u));
        y += lineH;
    }

    const int_t scroll = legacyMenuKeepVisible(m_selected, count, l.visibleRows, 0);
    for (int_t vi = 0; vi < l.visibleRows && scroll + vi < count; ++vi)
    {
        const Row &r = rows[static_cast<std::size_t>(scroll + vi)];
        legacyDrawMenuButton(mc, l.contentX, l.rowY(vi), l.contentW, l.rowH, r.label, scroll + vi == m_selected, r.enabled);
    }
    if (count > l.visibleRows)
        legacyDrawMenuScrollbar(l.panelX + l.panelW - 6, l.firstRowY, l.visibleRows * (l.rowH + l.rowGap), scroll,
                                l.visibleRows, count);

    drawRenameBox(true);
    drawHints(l.hintY);
}
