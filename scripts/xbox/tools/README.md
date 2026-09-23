# Xbox debugging tools

Helpers used to bring the port up under xemu and on a real console. Paths
inside some scripts point at the original developer's machine; adjust them.

| Tool | What it does |
|------|--------------|
| `escuchar_log.py` | Listens on UDP 9999 and prints the game log a console build sends when configured with `-DXBOX_NETLOG_HOST=<PC IP>` (saved to `netlog.txt`). |
| `gdbpeek.py` | Minimal gdb-remote client for xemu's gdbstub (`-gdb tcp:127.0.0.1:1235`). |
| `gdblog.py` | Dumps the in-memory log ring (`g_xboxLogRing`) and reports a kernel bug check. |
| `gdbstack.py` | At a bug check, scans the stack and resolves return addresses against the linker map. |
| `gdbthrow.py` | Breaks on every C++ throw and prints `what()` and the call chain. |
| `gdbstep.py`, `gdbring.py` | Break at PE addresses and dump registers / the log ring. |
| `autotest.ps1` + `autopilot.txt` | Builds an autopilot ISO (scripted controller, never deployed), boots it in xemu and dumps the log. |
| `run-xemu.ps1`, `captura.ps1` | Launch xemu / screenshot its window. |
| `vigilar_log.sh` | Polls the log ring every 30 s while someone plays. |

XBE addresses are the PE (map) addresses minus `0x3F0000`.
