"""Stops at given PE addresses (breakpoints) and dumps regs + stack words.
usage: python gdbstep.py <pe_addr_hex> [...]"""
import socket, sys, time
sys.path.insert(0, __file__.rsplit("\\", 1)[0])
from gdbpeek import Gdb
D = 0x3F0000
bps = [int(a, 16) - D for a in sys.argv[1:]]
g = Gdb()
print("stop:", g.cmd("?"), flush=True)
for b in bps:
    print("bp", hex(b), g.cmd(f"Z1,{b:x},1"), flush=True)
deadline = time.time() + 90
seen = 0
g.s.sendall(b"$c#63")
while time.time() < deadline and seen < len(bps) + 4:
    g.s.settimeout(max(1.0, deadline - time.time()))
    try:
        rep = g._read_packet()
    except socket.timeout:
        break
    if not rep.startswith(("T", "S")):
        continue
    r = g.regs()
    seen += 1
    esp = r["esp"]
    st = g.read(esp, 0x80) or b""
    words = [int.from_bytes(st[i:i+4], "little") for i in range(0, len(st), 4)]
    print(f"hit eip={r['eip']+D:08x} eax={r['eax']:08x} esp={esp:08x}", " ".join(f"{w:08x}" for w in words[:16]), flush=True)
    print("   bytes", st[0x40:0x80].hex(), flush=True)
    b = r["eip"]
    g.cmd(f"Z1,{b:x},1"); g.s.sendall(b"$s#73"); g._read_packet(); g.cmd(f"Z1,{b:x},1")
    g.s.sendall(b"$c#63")
for b in bps:
    g.cmd(f"Z1,{b:x},1")
g.s.sendall(b"$c#63")
