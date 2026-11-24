#pragma once

// Install / update over the network for the launcher (XboxLauncher.cpp).
//
// The console pulls the newest build itself from the "ultima" GitHub release:
// manifest.txt lists every file (path, size, SHA-256, asset name). Files are
// fetched one by one over HTTPS into <path>.part, checked, and only when all of
// them are good do they replace the installed ones. Files that already match
// are skipped, so an update only downloads what changed. No zip.
//
// A phone on the same network starts it: the launcher serves a small web page
// on port 80 (http://<console IP>/) with one button per version, and the
// project's GitHub page links there. The pad can start it too (Y).
namespace LauncherWifi
{
enum class State { Idle, Running, Done, Failed };

struct Progress
{
    State state;
    char version[16];        // what is being installed ("1.5.2", "1.2.5", "todo")
    char file[96];           // file being downloaded
    int fileIndex, fileCount;
    unsigned bytes, total;   // whole install, in bytes still to download
    char message[160];       // last status line (Spanish, shown on TV and phone)
    char build[64];          // "# build ..." line of the manifest
};

// Starts the network and the web server on a background thread. Never blocks.
void start();
// The console's address ("192.168.0.3"), or "" while there is none.
void address(char* out, int size);
void progress(Progress& out);
// Starts an install in the background; false if one is already running.
bool startInstall(const char* version);
// Back to Idle after a finished/failed install (the TV hides the result).
void clear();
// True once after an install finished, so the launcher re-reads its folders.
bool takeFinished();
} // namespace LauncherWifi
