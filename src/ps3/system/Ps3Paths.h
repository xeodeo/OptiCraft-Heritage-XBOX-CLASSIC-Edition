#pragma once

// Where the game lives on the PS3. Everything is under the title's own folder
// on the internal disk, /dev_hdd0/game/<title id>/USRDIR: the read-only game
// data (assets.pak, data/) and the writable state (worlds, options.txt) side by
// side, the way most homebrew does it. The folder is taken from argv[0] (the
// path the XMB launched EBOOT.BIN from) so a renamed title id still works.

namespace Ps3Paths
{
// Call once from main() with argv[0] (may be null).
void initialize(const char* argv0);

// "/dev_hdd0/game/OPTC00001/USRDIR", no trailing slash.
const char* installDir();

// Where worlds and options are written. Same as installDir() today.
const char* writableDir();
}
