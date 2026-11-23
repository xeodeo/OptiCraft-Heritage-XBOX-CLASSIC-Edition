#pragma once

#include "net/minecraft/src/GuiScreen.h"
#include <string>
#include <vector>

class EntityChocobo;
class EntityPlayer;
class GuiTextField;

// ChocoCraft 4.1.5 Chocopedia (GuiChocopedia + GuiNameChocobo + GuiSelectNewOwner +
// GuiChangeOwner in one screen), in the Java look and in the Legacy (LCE) look, with
// the pad. Without a chocobo it is the book; on a tamed chocobo it shows its data
// (name, owner, health, gender) and the owner's options: rename, show/hide the name,
// follow/wander/stay, give it to another player, drop saddle and bags.
class GuiChocopedia : public GuiScreen
{
public:
    GuiChocopedia(EntityChocobo *chocobo, EntityPlayer *player, int playerIndex);
    ~GuiChocopedia() override;

    // Opens it for that player (P1 or the split screen P2).
    static void open(EntityPlayer *player, EntityChocobo *chocobo);
    static bool isScreenOpenFor(EntityPlayer *player);

    void initGui() override;
    void onGuiClosed() override;
    void updateScreen() override;
    void actionPerformed(GuiButton *button) override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void handleSpecializedMenuInput() override;
    bool usesSpecializedMenuNavigation() const override;
    bool doesGuiPauseGame() override { return false; }

protected:
    void keyTyped(char_t c, int_t key) override;

private:
    enum class Mode { Book, Info, SelectOwner, ConfirmOwner };
    enum Action
    {
        ACT_RENAME = 0,
        ACT_HIDENAME,
        ACT_FOLLOW,
        ACT_CHANGE_OWNER,
        ACT_DROP_GEAR,
        ACT_CONFIRM,
        ACT_ACCEPT,
        ACT_CANCEL,
        ACT_PREV_PAGE,
        ACT_NEXT_PAGE,
        ACT_DONE,
        ACT_PLAYER_BASE = 100
    };
    struct Row
    {
        int_t action;
        std::string label;
        bool enabled;
    };

    bool isLegacyMode() const;
    bool spanish() const;
    void buildRows(std::vector<Row> &rows) const;
    void rebuildJavaButtons();
    void activate(int_t action);
    void setMode(Mode mode);
    void turnPage(int_t delta);
    int_t pageCount() const;
    void beginRename();
    void commitRename();
    void cancelRename();
    void closeScreen();
    void playClick(const char *sound);
    std::vector<std::string> infoLines() const;
    void drawJava(int_t mouseX, int_t mouseY, float_t partialTick);
    void drawLegacy(float_t partialTick);
    void drawRenameBox(bool legacy);
    void drawHints(int_t hintY);

    EntityChocobo *m_chocobo;
    EntityPlayer *m_player;
    int m_playerIndex;
    Mode m_mode;
    int_t m_page;
    int_t m_selected;
    bool m_renaming;
    bool m_openLatch;
    int_t m_javaRowsBuilt;
    Mode m_javaModeBuilt;
    GuiTextField *m_nameField;
    std::vector<std::string> m_players;
    std::string m_newOwner;
};
