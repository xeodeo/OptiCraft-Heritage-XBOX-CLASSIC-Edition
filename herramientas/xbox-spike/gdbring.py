"""Break at PE addresses; at each hit print ring counter + ring text so far."""
import socket, sys, time
sys.path.insert(0, __file__.rsplit("\\", 1)[0])
from gdbpeek import Gdb
D = 0x3F0000
RING = 0x801eb0 - D; CNT = 0x811eb0 - D
bps = [int(a, 16) - D for a in sys.argv[1:]]
g = Gdb(); g.cmd("?")
for b in bps: g.cmd(f"Z1,{b:x},1")
g.s.sendall(b"$c#63"); deadline = time.time() + 60; n = 0
while time.time() < deadline and n < 6:
    g.s.settimeout(max(1.0, deadline - time.time()))
    try: rep = g._read_packet()
    except socket.timeout: break
    if not rep.startswith(("T", "S")): continue
    n += 1; r = g.regs()
    cnt = int.from_bytes(g.read(CNT, 4), "little")
    txt = (g.read(RING, min(cnt, 2000)) or b"")
    print(f"hit {r['eip']+D:08x} cnt={cnt}\n{txt.decode('latin-1')}\n----", flush=True)
    b = r["eip"]; g.cmd(f"z1,{b:x},1"); g.s.sendall(b"$s#73"); g._read_packet(); g.cmd(f"Z1,{b:x},1"); g.s.sendall(b"$c#63")
for b in bps: g.cmd(f"z1,{b:x},1")
g.s.sendall(b"$c#63")
