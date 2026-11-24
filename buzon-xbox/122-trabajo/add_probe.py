import sys, os
d = sys.argv[1]  # src/mods/adminmode
here = os.path.dirname(os.path.abspath(__file__))
p = os.path.join(d, 'AdminModeMod.h')
s = open(p, encoding='utf-8').read()
old = 'private:\n    void loadConfig();'
assert old in s
s = s.replace(old, '#if XBOX_AUTOPILOT\n    void probeTick(); // PROBE122 (temporary)\n#endif\nprivate:\n    void loadConfig();')
open(p, 'w', encoding='utf-8').write(s)

p = os.path.join(d, 'AdminModeMod.cpp')
s = open(p, encoding='utf-8').read()
old = 'void AdminModeMod::onTick()\n{\n    Minecraft *mc = m_mc;\n    if (mc == nullptr)\n        return;\n'
assert old in s
s = s.replace(old, old + '#if XBOX_AUTOPILOT\n    probeTick(); // PROBE122\n#endif\n')
old = '    bool allowed = !mc->theWorld->multiplayerWorld;\n'
assert s.count(old) == 1
s = s.replace(old, old + '#if XBOX_AUTOPILOT\n    extern bool g_probeForceRemote; // PROBE122\n    if (g_probeForceRemote)\n        allowed = false;\n#endif\n')
s += open(os.path.join(here, 'probe_code.cpp'), encoding='utf-8').read()
open(p, 'w', encoding='utf-8').write(s)
print('ok')
