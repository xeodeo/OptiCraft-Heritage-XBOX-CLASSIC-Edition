#pragma once

#include "GuiScreen.h"

#include <memory>
#include <string>
#include <vector>

class GuiTextField;

// "Download from the internet": Modrinth resource packs for this game
// version, like a store. Opens straight onto ready lists (most downloaded,
// newest, updated, most followed) with the pack icons, switched with
// White/Black, paged with left/right; X searches by name with the on-screen
// keyboard. A opens a pack's card (newest file for this version, size,
// compatibility hint); A again downloads it to the texture pack folder in
// pieces (progress bar, B cancels), and 1.5-format / 64x packs are converted
// for the console right after (TexDlConvert). Network and conversion run on
// worker threads; the render thread only draws and polls.
//
// Memory: one JSON page (10 packs, ~40 KB), the icon being decoded on the
// worker, and at most 12 icon textures of 32x32 (4 KB each), freed when the
// screen closes, together with the shared TLS state.
class GuiTexturePackDownload : public GuiScreen
{
public:
    explicit GuiTexturePackDownload(GuiScreen *parent);
    ~GuiTexturePackDownload() override;

    void initGui() override;
    void onGuiClosed() override;
    void updateScreen() override;
    void drawScreen(int_t mouseX, int_t mouseY, float_t partialTick) override;
    void keyTyped(char_t c, int_t key) override;
    bool usesSpecializedMenuNavigation() const override;
    void handleSpecializedMenuInput() override;
    bool doesGuiPauseGame() override { return true; }

    struct PageJob;
    struct VersionJob;
    struct DownloadJob;

private:
    enum class Mode
    {
        Browse,
        Detail,
        Downloading,
        Finished
    };

    struct IconEntry
    {
        std::string projectId;
        int_t texture = -1;
        unsigned lastUse = 0;
    };

    bool isLegacy() const;
    bool isSpanish() const;
    int tabCount() const;
    std::string tabLabel(int tab, bool shortLabel) const;
    void requestPage();
    void cancelJobs();
    void freeIcons();
    void leave();
    void pollJobs();
    int_t iconFor(const std::string &projectId);
    void storeIcon(const std::string &projectId, const std::vector<unsigned char> &rgba);
    void openDetail();
    void startDownload();
    void useInstalledPack();
    void beginSearch();
    void endSearch(bool run);
    void changeTab(int delta);
    void changePage(int delta);
    void play(const char *sound, float pitch = 1.0f);

    void drawBrowse(float_t partialTick);
    void drawDetail();
    void drawProgress();
    void drawFinished();
    void drawFrame(int_t &x, int_t &y, int_t &w, int_t &h, int_t rows, int_t headerH, int_t footerH, float_t partialTick);
    void drawIcon(int_t texture, int_t x, int_t y, int_t size);
    void drawHints(const std::string *buttons, const std::string *actions, int_t count);

    GuiScreen *parentScreen;
    GuiTextField *searchField = nullptr;
    bool searching = false;
    bool inputLatch = true;
    Mode mode = Mode::Browse;

    int tab = 0;            // 0 = made for this version (filtered), 1..4 lists, 5 = search results
    int page = 0;
    int selected = 0;
    int scroll = 0;
    std::string query;

    std::shared_ptr<PageJob> pageJob;
    std::shared_ptr<VersionJob> versionJob;
    std::shared_ptr<DownloadJob> downloadJob;

    // Current page (copied from the job when it lands).
    struct Hit;
    std::vector<std::shared_ptr<Hit>> hits;
    // Tab 0 fetches every hit for the version once, keeps the old-format ones and
    // pages through them locally.
    std::vector<std::shared_ptr<Hit>> allHits;
    bool localPages = false;
    int verifyDone = 0;
    int verifyTotal = 0;
    void showLocalPage();
    int totalHits = 0;
    bool pageLoading = false;
    std::string pageError;

    std::vector<IconEntry> icons;
    unsigned iconClock = 0;

    // Detail / download state.
    std::shared_ptr<Hit> detailHit;
    std::string statusText;
    int_t statusColor = 0;
    std::string installedName;
    unsigned animTicks = 0;
};
