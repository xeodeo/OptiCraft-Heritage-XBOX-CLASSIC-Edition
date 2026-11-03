#include "GuiScreenBook.h"
#include "GuiButtonNextPage.h"
#include "GuiButton.h"
#include "GuiTextField.h"
#include "EntityPlayer.h"
#include "ItemStack.h"
#include "Item.h"
#include "NBTTagCompound.h"
#include "NBTTagList.h"
#include "NBTTagString.h"
#include "StatCollector.h"
#include "ChatAllowedCharacters.h"
#include "Minecraft.h"
#include "RenderEngine.h"
#include "FontRenderer.h"
#include "World.h"
#include "NetClientHandler.h"
#include "Packet250CustomPayload.h"
#include "java/String.h"
#include "pc/lwjgl/Keyboard.h"
#include "platform/RenderAPI.h"

GuiScreenBook::GuiScreenBook(EntityPlayer *player, ItemStack *book, bool isUnsigned)
    : editingPlayer(player)
    , itemstackBook(book)
    , bookIsUnsigned(isUnsigned)
    , bookModified(false)
    , editingTitle(false)
    , updateCount(0)
    , bookImageWidth(192)
    , bookImageHeight(192)
    , bookTotalPages(1)
    , currPage(0)
    , bookPages(nullptr)
    , bookTitle("")
    , buttonNextPage(nullptr)
    , buttonPreviousPage(nullptr)
    , buttonDone(nullptr)
    , buttonSign(nullptr)
    , buttonFinalize(nullptr)
    , buttonCancel(nullptr)
    , textInput(nullptr)
{
    if (book != nullptr && book->hasTagCompound())
    {
        NBTTagCompound *comp = book->getTagCompound();
        NBTTagList *pages = comp->getTagList("pages");
        if (pages != nullptr)
        {
            bookPages = dynamic_cast<NBTTagList *>(pages->copy());
            if (bookPages != nullptr)
            {
                bookTotalPages = bookPages->tagCount();
                if (bookTotalPages < 1)
                {
                    bookTotalPages = 1;
                }
            }
        }
    }

    if (bookPages == nullptr && isUnsigned)
    {
        bookPages = new NBTTagList("pages");
        bookPages->appendTag(new NBTTagString("1", ""));
        bookTotalPages = 1;
    }
}

GuiScreenBook::~GuiScreenBook()
{
    if (textInput != nullptr)
    {
        textInput->setFocused(false);
    }
    delete textInput;
}

void GuiScreenBook::updateScreen()
{
    GuiScreen::updateScreen();
    updateCount++;
}

void GuiScreenBook::initGui()
{
    controlList.clear();
    lwjgl::Keyboard::enableRepeatEvents(true);

    if (bookIsUnsigned)
    {
        controlList.push_back(buttonSign = new GuiButton(3, width / 2 - 100, 4 + bookImageHeight, 98, 20, StatCollector::translateToLocal("book.signButton")));
        controlList.push_back(buttonDone = new GuiButton(0, width / 2 + 2, 4 + bookImageHeight, 98, 20, StatCollector::translateToLocal("gui.done")));
        controlList.push_back(buttonFinalize = new GuiButton(5, width / 2 - 100, 4 + bookImageHeight, 98, 20, StatCollector::translateToLocal("book.finalizeButton")));
        controlList.push_back(buttonCancel = new GuiButton(4, width / 2 + 2, 4 + bookImageHeight, 98, 20, StatCollector::translateToLocal("gui.cancel")));
    }
    else
    {
        controlList.push_back(buttonDone = new GuiButton(0, width / 2 - 100, 4 + bookImageHeight, 200, 20, StatCollector::translateToLocal("gui.done")));
    }

    int x = (width - bookImageWidth) / 2;
    int y = 2;
    controlList.push_back(buttonNextPage = new GuiButtonNextPage(1, x + 120, y + 154, true));
    controlList.push_back(buttonPreviousPage = new GuiButtonNextPage(2, x + 38, y + 154, false));

    delete textInput;
    textInput = new GuiTextField(this, fontRenderer, 0, 0, 1, 1, "");
    if (bookIsUnsigned)
    {
        textInput->setMaxStringLength(256);
        textInput->setText(func_74158_i());
        textInput->setFocused(true);
    }

    updateButtons();
}

void GuiScreenBook::onGuiClosed()
{
    lwjgl::Keyboard::enableRepeatEvents(false);
    if (textInput != nullptr)
    {
        textInput->setFocused(false);
    }
}

void GuiScreenBook::updateButtons()
{
    if (buttonNextPage != nullptr)
    {
        buttonNextPage->enabled2 = !editingTitle && (currPage < bookTotalPages - 1 || bookIsUnsigned);
    }
    if (buttonPreviousPage != nullptr)
    {
        buttonPreviousPage->enabled2 = !editingTitle && currPage > 0;
    }
    if (buttonDone != nullptr)
    {
        buttonDone->enabled2 = !bookIsUnsigned || !editingTitle;
    }
    if (bookIsUnsigned)
    {
        if (buttonSign != nullptr)
        {
            buttonSign->enabled2 = !editingTitle;
        }
        if (buttonCancel != nullptr)
        {
            buttonCancel->enabled2 = editingTitle;
        }
        if (buttonFinalize != nullptr)
        {
            buttonFinalize->enabled2 = editingTitle;
            buttonFinalize->enabled = !bookTitle.empty();
        }
    }
}

void GuiScreenBook::sendBookToServer(bool isSigning)
{
    if (bookIsUnsigned && bookModified && itemstackBook != nullptr)
    {
        if (bookPages != nullptr)
        {
            while (bookPages->tagCount() > 1)
            {
                NBTTagString *str = dynamic_cast<NBTTagString *>(bookPages->tagAt(bookPages->tagCount() - 1));
                if (str != nullptr && !str->data.empty())
                {
                    break;
                }
                bookPages->removeTag(bookPages->tagCount() - 1);
            }

            if (itemstackBook->hasTagCompound())
            {
                itemstackBook->getTagCompound()->setTag("pages", bookPages);
            }
            else
            {
                itemstackBook->setTagInfo("pages", bookPages);
            }

            if (isSigning)
            {
                std::string author = editingPlayer != nullptr ? editingPlayer->username : "Player";
                itemstackBook->setTagInfo("author", new NBTTagString("author", author));
                itemstackBook->setTagInfo("title", new NBTTagString("title", bookTitle));
                if (Item::writtenBook != nullptr)
                {
                    itemstackBook->itemID = Item::writtenBook->shiftedIndex;
                }
            }
        }
    }
}

void GuiScreenBook::actionPerformed(GuiButton *button)
{
    if (button == nullptr || !button->enabled)
    {
        return;
    }

    if (button->id == 0)
    {
        mc->displayGuiScreen(nullptr);
        sendBookToServer(false);
    }
    else if (button->id == 3 && bookIsUnsigned)
    {
        editingTitle = true;
        if (textInput != nullptr)
        {
            textInput->setMaxStringLength(16);
            textInput->setText(bookTitle);
        }
    }
    else if (button->id == 1)
    {
        if (currPage < bookTotalPages - 1)
        {
            currPage++;
        }
        else if (bookIsUnsigned)
        {
            addNewPage();
            if (currPage < bookTotalPages - 1)
            {
                currPage++;
            }
        }
        if (textInput != nullptr)
        {
            textInput->setMaxStringLength(256);
            textInput->setText(func_74158_i());
        }
    }
    else if (button->id == 2)
    {
        if (currPage > 0)
        {
            currPage--;
        }
        if (textInput != nullptr)
        {
            textInput->setMaxStringLength(256);
            textInput->setText(func_74158_i());
        }
    }
    else if (button->id == 5 && editingTitle)
    {
        sendBookToServer(true);
        mc->displayGuiScreen(nullptr);
    }
    else if (button->id == 4 && editingTitle)
    {
        editingTitle = false;
        if (textInput != nullptr)
        {
            textInput->setMaxStringLength(256);
            textInput->setText(func_74158_i());
        }
    }

    updateButtons();
}

void GuiScreenBook::addNewPage()
{
    if (bookPages != nullptr && bookPages->tagCount() < 50)
    {
        bookPages->appendTag(new NBTTagString(std::to_string(bookTotalPages + 1), ""));
        bookTotalPages++;
        bookModified = true;
    }
}

void GuiScreenBook::keyTyped(char_t c, int_t key)
{
    GuiScreen::keyTyped(c, key);
    if (bookIsUnsigned)
    {
        if (editingTitle)
        {
            func_74162_c(c, key);
        }
        else
        {
            keyTypedInBook(c, key);
        }
    }
}

void GuiScreenBook::keyTypedInBook(char_t c, int_t key)
{
    if (key == 14) // Backspace
    {
        std::string s = func_74158_i();
        if (!s.empty())
        {
            func_74159_a(String::removeLastUtf16Unit(s));
        }
    }
    else if (key == 28) // Enter
    {
        func_74160_b("\n");
    }
    else if (c != 0 && ChatAllowedCharacters::isAllowedCharacter(c))
    {
        std::string appendStr;
        appendStr.push_back((char)c);
        func_74160_b(appendStr);
    }

    if (textInput != nullptr)
    {
        textInput->setText(func_74158_i());
    }
}

void GuiScreenBook::func_74162_c(char_t c, int_t key)
{
    if (key == 14) // Backspace
    {
        if (!bookTitle.empty())
        {
            bookTitle = String::removeLastUtf16Unit(bookTitle);
            updateButtons();
        }
    }
    else if (key == 28) // Enter
    {
        if (!bookTitle.empty())
        {
            sendBookToServer(true);
            mc->displayGuiScreen(nullptr);
        }
    }
    else if (bookTitle.length() < 16 && ChatAllowedCharacters::isAllowedCharacter(c))
    {
        bookTitle.push_back((char)c);
        updateButtons();
        bookModified = true;
    }

    if (textInput != nullptr)
    {
        textInput->setText(bookTitle);
    }
}

std::string GuiScreenBook::func_74158_i()
{
    if (bookPages != nullptr && currPage >= 0 && currPage < bookPages->tagCount())
    {
        NBTTagString *str = dynamic_cast<NBTTagString *>(bookPages->tagAt(currPage));
        if (str != nullptr)
        {
            return str->data;
        }
    }
    return "";
}

void GuiScreenBook::func_74159_a(const std::string &text)
{
    if (bookPages != nullptr && currPage >= 0 && currPage < bookPages->tagCount())
    {
        NBTTagString *str = dynamic_cast<NBTTagString *>(bookPages->tagAt(currPage));
        if (str != nullptr)
        {
            str->data = text;
            bookModified = true;
        }
    }
}

void GuiScreenBook::func_74160_b(const std::string &text)
{
    std::string cur = func_74158_i();
    std::string next = cur + text;
    int splitWidth = fontRenderer->splitStringWidth(next + "\xc2\xa7""0_", 118);
    if (splitWidth <= 118 && next.length() < 256)
    {
        func_74159_a(next);
    }
}

void GuiScreenBook::drawScreen(int_t mouseX, int_t mouseY, float_t partialTicks)
{
    renderColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    mc->renderEngine->bindTexture(mc->renderEngine->getTexture("/gui/book.png"));
    int x = (width - bookImageWidth) / 2;
    int y = 2;
    drawTexturedModalRect(x, y, 0, 0, bookImageWidth, bookImageHeight);

    if (editingTitle)
    {
        std::string titleStr = bookTitle;
        if (bookIsUnsigned)
        {
            if (updateCount / 6 % 2 == 0)
            {
                titleStr += "\xc2\xa7""0_";
            }
            else
            {
                titleStr += "\xc2\xa7""7_";
            }
        }

        std::string editTitleHeader = StatCollector::translateToLocal("book.editTitle");
        int headerWidth = fontRenderer->getStringWidth(editTitleHeader);
        fontRenderer->drawString(editTitleHeader, x + 36 + (116 - headerWidth) / 2, y + 16 + 16, 0);

        int titleWidth = fontRenderer->getStringWidth(titleStr);
        fontRenderer->drawString(titleStr, x + 36 + (116 - titleWidth) / 2, y + 48, 0);

        std::string authorStr = editingPlayer != nullptr ? editingPlayer->username : "Player";
        std::string byAuthor = StatCollector::translateToLocalFormatted("book.byAuthor", authorStr);
        int authorWidth = fontRenderer->getStringWidth(byAuthor);
        fontRenderer->drawString("\xc2\xa7""8" + byAuthor, x + 36 + (116 - authorWidth) / 2, y + 48 + 10, 0);

        std::string warning = StatCollector::translateToLocal("book.finalizeWarning");
        fontRenderer->drawSplitString(warning, x + 36, y + 80, 116, 0);
    }
    else
    {
        std::string pageIndicator = StatCollector::translateToLocalFormatted("book.pageIndicator", std::to_string(currPage + 1), std::to_string(bookTotalPages));
        std::string pageText = func_74158_i();

        if (bookIsUnsigned)
        {
            if (updateCount / 6 % 2 == 0)
            {
                pageText += "\xc2\xa7""0_";
            }
            else
            {
                pageText += "\xc2\xa7""7_";
            }
        }

        int indWidth = fontRenderer->getStringWidth(pageIndicator);
        fontRenderer->drawString(pageIndicator, x - indWidth + bookImageWidth - 44, y + 16, 0);
        fontRenderer->drawSplitString(pageText, x + 36, y + 16 + 16, 116, 0);
    }

    GuiScreen::drawScreen(mouseX, mouseY, partialTicks);
}
