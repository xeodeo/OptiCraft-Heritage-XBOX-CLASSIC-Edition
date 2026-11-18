"""At a bug check, scan the stack for return addresses inside the XBE and
resolve them against the map (rough call chain). usage: gdbstack.py <map> [bytes]"""
import sys
sys.path.insert(0, __file__.rsplit("\\", 1)[0])
from gdbpeek import Gdb, load_map, resolve
D = 0x3F0000
syms = load_map(sys.argv[1]); keys = [s[0] for s in syms]
n = int(sys.argv[2]) if len(sys.argv) > 2 else 4096
g = Gdb(); g.cmd("?"); r = g.regs()
print(f"eip={r['eip']:08x} esp={r['esp']:08x} ecx={r['ecx']:08x} edx={r['edx']:08x}")
lo, hi = keys[0] - D, keys[-1] - D
data = b""
for off in range(0, n, 1024):
    data += g.read(r["esp"] + off, 1024) or b"\0" * 1024
seen = 0
for i in range(0, len(data) - 3, 4):
    w = int.from_bytes(data[i:i + 4], "little")
    if lo <= w < hi:
        name = resolve(syms, w + D, keys)
        if name and not name.startswith("?"[:0] + "__tls"):
            print(f"  +{i:04x} {w:08x} {name}")
            seen += 1
            if seen > 40: break
g.s.sendall(b"$c#63")
