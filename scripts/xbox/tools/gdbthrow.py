"""Catches every C++ throw in an Xbox title running under xemu's gdbstub.

Sets a breakpoint on _CxxThrowException (address from the linker map), and on
each hit prints the exception's what() string (MSVC std::exception layout:
vptr, then const char* _What) and the throwing call chain resolved against the
map, then resumes. Runs until `seconds` elapse or `max_hits` throws are seen.

usage: python gdbthrow.py <map file> [seconds] [max_hits]
"""
import bisect
import re
import socket
import sys
import time

sys.path.insert(0, __file__.rsplit("\\", 1)[0])
from gdbpeek import Gdb, REGS, load_map, resolve  # noqa: E402

XBE_DELTA = 0x3F0000  # PE link base 0x400000 -> XBE base 0x10000


def read_u32(g, addr):
    b = g.read(addr, 4)
    return int.from_bytes(b, "little") if b else None


def read_cstr(g, addr, limit=200):
    if not addr:
        return None
    b = g.read(addr, limit)
    if not b:
        return None
    return b.split(b"\0", 1)[0].decode("latin-1", "replace")


def main():
    syms = load_map(sys.argv[1])
    keys = [s[0] for s in syms]
    seconds = int(sys.argv[2]) if len(sys.argv) > 2 else 180
    max_hits = int(sys.argv[3]) if len(sys.argv) > 3 else 20

    throw_pe = next(a for a, n, _ in syms if n == "__CxxThrowException@8")
    bp = throw_pe - XBE_DELTA
    image_lo, image_hi = keys[0] - XBE_DELTA, keys[-1] - XBE_DELTA + 0x100000

    def where(xbe_addr):
        return resolve(syms, xbe_addr + XBE_DELTA, keys)

    g = Gdb()
    print("stop reason:", g.cmd("?"), flush=True)
    print("breakpoint:", g.cmd(f"Z0,{bp:x},1"), f"at {bp:08x}", flush=True)
    g.s.settimeout(None)
    deadline = time.time() + seconds
    hits = 0
    g.s.sendall(b"$c#63")
    while time.time() < deadline and hits < max_hits:
        g.s.settimeout(max(1.0, deadline - time.time()))
        try:
            reply = g._read_packet()
        except socket.timeout:
            break
        if not reply.startswith(("T", "S")):
            continue
        r = g.regs()
        if r["eip"] != bp:
            g.s.sendall(b"$c#63")
            continue
        hits += 1
        esp = r["esp"]
        ret = read_u32(g, esp)
        obj = read_u32(g, esp + 4)
        what = read_cstr(g, read_u32(g, obj + 4)) if obj else None
        print(f"--- throw #{hits}: what={what!r}", flush=True)
        print(f"    thrown from {where(ret) if ret else '?'}", flush=True)
        stack = g.read(esp, 512) or b""
        shown = 0
        for i in range(4, len(stack), 4):
            w = int.from_bytes(stack[i:i + 4], "little")
            if image_lo <= w < image_hi:
                name = where(w)
                if "?" not in name[:1] and shown < 10:
                    print(f"      {name}", flush=True)
                    shown += 1
        # step over the breakpoint, re-arm it, continue
        g.cmd(f"z0,{bp:x},1")
        g.s.sendall(b"$s#73")
        g._read_packet()
        g.cmd(f"Z0,{bp:x},1")
        g.s.sendall(b"$c#63")
    print(f"done: {hits} throws seen", flush=True)
    try:
        g.cmd(f"z0,{bp:x},1")
        g.s.sendall(b"$c#63")
    except Exception:
        pass


if __name__ == "__main__":
    main()
