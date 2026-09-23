#pragma once

namespace XboxWritableRoot
{
// Drive the title writes to: "T:" (XAPI maps it to E:\TDATA\<title id>), or
// "Z:" (the title's utility/cache partition) when T: cannot be written.
// Decided on first call; call it once early in main().
const char* get();
}
