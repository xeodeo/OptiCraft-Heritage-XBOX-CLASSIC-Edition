import sys, re
# strip_probe.py <AdminModeMod.cpp in> <AdminModeMod.h in> <out.cpp> <out.h>
cpp = open(sys.argv[1], encoding='utf-8', newline='').read().replace('\r\n', '\n')
h = open(sys.argv[2], encoding='utf-8', newline='').read().replace('\r\n', '\n')
i = cpp.find('\n#if XBOX_AUTOPILOT\n// ===== PROBE122')
if i >= 0:
    cpp = cpp[:i + 1]
# small "#if XBOX_AUTOPILOT ... PROBE122 ... #endif" blocks
block = re.compile(r'#if XBOX_AUTOPILOT\n(?:[^\n]*\n){1,6}?#endif\n')
def drop(m):
    return '' if 'PROBE122' in m.group(0) else m.group(0)
cpp = block.sub(drop, cpp)
h = block.sub(drop, h)
assert 'PROBE' not in cpp and 'probe' not in cpp.lower() and 'probe' not in h.lower(), 'probe left'
open(sys.argv[3], 'w', encoding='utf-8', newline='').write(cpp)
open(sys.argv[4], 'w', encoding='utf-8', newline='').write(h)
print('stripped')
