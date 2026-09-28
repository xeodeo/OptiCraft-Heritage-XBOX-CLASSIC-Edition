#pragma once

// Logs crashes that bypass the red crash screen (access violations on any
// thread, C++ exceptions escaping worker threads), then parks the thread.
namespace XboxCrashLog
{
void install();
}
