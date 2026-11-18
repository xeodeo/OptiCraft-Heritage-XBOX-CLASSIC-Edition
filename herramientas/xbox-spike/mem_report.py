# -*- coding: utf-8 -*-
"""Resume freeKB / xbox.mem del netlog: una fila por muestra."""
import re, sys
path = sys.argv[1] if len(sys.argv) > 1 else r"C:\Users\xeodeo\Desktop\Minecraft en xboox clasico\xbox-spike\netlog.txt"
fpu = re.compile(r"frame=(\d+).*?freeKB=(-?\d+)")
mem = re.compile(r"lists=(\d+) \((\d+)KB\) vbPools=(\d+) textures=(\d+) \((\d+)KB\)")
rows = []
pend = None
for line in open(path, encoding="utf-8", errors="replace"):
    if line.startswith("===="):
        rows.append(("--- sesion ---",))
        continue
    m = fpu.search(line)
    if m:
        pend = (int(m.group(1)), int(m.group(2)))
        continue
    m = mem.search(line)
    if m and pend:
        rows.append((pend[0], pend[1], int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))))
        pend = None
print("frame  freeKB  freeMB  lists  listKB  vbPools  tex  texKB   dFree")
prev = None
for r in rows:
    if len(r) == 1:
        print(r[0]); prev = None; continue
    fr, free, lists, lkb, vb, tex, tkb = r
    d = "" if prev is None else "%+d" % (free - prev)
    prev = free
    print("%6d %7d %6.1f %6d %7d %8d %5d %6d  %s" % (fr, free, free/1024.0, lists, lkb, vb, tex, tkb, d))

# Las muertes se detectan aparte: busca en xbox.player un salto de mas de 50
# bloques que acabe a menos de 1 bloque del punto de reaparicion.
