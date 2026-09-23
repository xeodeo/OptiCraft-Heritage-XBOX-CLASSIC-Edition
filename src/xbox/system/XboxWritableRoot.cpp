// XboxWritableRoot.cpp — picks the drive the title saves to.
//
// XAPI maps T: to E:\TDATA\<title id> for every title, so it is the normal
// home for worlds, options and the debug log. Should a launcher or a damaged
// E: partition leave it unwritable, fall back to the utility drive Z:, which
// the title mounts itself (and the console may wipe between titles).
#ifdef XBOX_PLATFORM

#include "xbox/XboxXtl.h"

#include "xbox/system/XboxWritableRoot.h"

namespace
{
bool canWrite(const char* probePath)
{
    HANDLE file = CreateFileA(probePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    CloseHandle(file);
    DeleteFileA(probePath);
    return true;
}

const char* choose()
{
    if (canWrite("T:\writable.tmp"))
        return "T:";
    if (XMountUtilityDrive(FALSE) && canWrite("Z:\writable.tmp"))
        return "Z:";
    return "T:";
}
} // namespace

namespace XboxWritableRoot
{
const char* get()
{
    static const char* root = choose();
    return root;
}
} // namespace XboxWritableRoot

#endif // XBOX_PLATFORM
