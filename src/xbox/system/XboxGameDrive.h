#pragma once

// Points D: at the folder of the running XBE.
//
// The launcher (src/xbox/tools/XboxLauncher.cpp) starts game XBEs from
// per-version folders (D:\1.2.5\OptiCraft.xbe, D:\1.5.2\...), and the title it
// launches keeps the launcher's D:, the folder above. Every resource path in
// the game is spelled from D: (assets.pak, data\, texturepacks\), so main()
// relinks D: to the XBE's own folder before anything opens a file. Started from
// the folder D: already names, this changes nothing.
namespace XboxGameDrive
{
void mapToImageFolder();
}
