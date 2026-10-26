#pragma once

#include "GuiScreen.h"
#include <string>

class EntityPlayer;
class ItemStack;
class NBTTagList;
class GuiButton;
class GuiButtonNextPage;
class GuiTextField;

// net.minecraft.src.GuiScreenBook
class GuiScreenBook : public GuiScreen
{
public:
    GuiScreenBook(EntityPlayer *player, ItemStack *book, bool isUnsigned);
    ~GuiScreenBook() override;

    void updateScreen() override;
    void initGui() override;
    void onGuiClosed() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTicks) override;

protected:
    void actionPerformed(GuiButton *button) override;
    void keyTyped(char_t c, int_t key) override;

private:
    void updateButtons();
    void sendBookToServer(bool isSigning);
    void addNewPage();
    void keyTypedInBook(char_t c, int_t key);
    void func_74162_c(char_t c, int_t key);
    std::string func_74158_i();
    void func_74159_a(const std::string &text);
    void func_74160_b(const std::string &text);

    EntityPlayer *editingPlayer = nullptr;
    ItemStack *itemstackBook = nullptr;
    bool bookIsUnsigned = false;
    bool bookModified = false;
    bool editingTitle = false;
    int updateCount = 0;
    int bookImageWidth = 192;
    int bookImageHeight = 192;
    int bookTotalPages = 1;
    int currPage = 0;
    NBTTagList *bookPages = nullptr;
    std::string bookTitle = "";

    GuiButtonNextPage *buttonNextPage = nullptr;
    GuiButtonNextPage *buttonPreviousPage = nullptr;
    GuiButton *buttonDone = nullptr;
    GuiButton *buttonSign = nullptr;
    GuiButton *buttonFinalize = nullptr;
    GuiButton *buttonCancel = nullptr;

    GuiTextField *textInput = nullptr;
};
