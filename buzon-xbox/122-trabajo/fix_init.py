import sys
p = sys.argv[1]
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'
old = '    m_mc = mc;' + nl + '    loadConfig();' + nl
assert s.count(old) == 1
new = old + '#if XBOX_AUTOPILOT' + nl + '    if (FILE *f = std::fopen("D:\\\\probe122.txt", "r")) { std::fclose(f); m_enabled = true; } // PROBE122' + nl + '#endif' + nl
s = s.replace(old, new)
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')
