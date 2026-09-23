"""Dumps the in-memory game log (src/xbox/system/XboxLogRing.cpp) from a title
running under xemu's gdbstub. Also reports whether the CPU sits in a kernel
bug check. usage: python gdblog.py <map file> [tail_bytes]
"""
import sys

sys.path.insert(0, __file__.rsplit("\\", 1)[0])
from gdbpeek import Gdb, load_map, resolve  # noqa: E402

XBE_DELTA = 0x3F0000
RING_SIZE = 64 * 1024


def main():
    syms = load_map(sys.argv[1])
    keys = [s[0] for s in syms]
    tail = int(sys.argv[2]) if len(sys.argv) > 2 else 12000
    addr = {n: a - XBE_DELTA for a, n, _ in syms if n in ("_g_xboxLogRing", "_g_xboxLogRingWrite")}
    g = Gdb()
    g.cmd("?")
    r = g.regs()
    if r["eip"] >= 0x80000000:
        print(f"[kernel] eip={r['eip']:08x} eax={r['eax']:08x} ecx={r['ecx']:08x} edx={r['edx']:08x}"
              f" -> {resolve(syms, r['edx'] + XBE_DELTA, keys) if r['edx'] < 0x80000000 else ''}")
    total = int.from_bytes(g.read(addr["_g_xboxLogRingWrite"], 4), "little")
    ring = b""
    for off in range(0, RING_SIZE, 2048):
        ring += g.read(addr["_g_xboxLogRing"] + off, 2048) or b"\0" * 2048
    if total <= RING_SIZE:
        text = ring[:total]
    else:
        start = total % RING_SIZE
        text = ring[start:] + ring[:start]
    print(f"[log] {total} bytes written; last {min(tail, len(text))}:")
    print(text[-tail:].decode("latin-1", "replace"))
    g.s.sendall(b"$c#63")


if __name__ == "__main__":
    main()
