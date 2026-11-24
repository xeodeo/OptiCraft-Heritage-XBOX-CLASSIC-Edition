import re, sys
root = sys.argv[1] + '/src/'
for rel in ['net/minecraft/src/World.cpp', 'client/Minecraft.cpp']:
    p = root + rel
    s = open(p, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    # deleteWorldOwnedEntity probe block back to the original
    s = re.sub(r'#include <intrin\.h>\r?\n__declspec\(noinline\) static void deleteWorldOwnedEntity\(Entity \*entity\)\r?\n\{\r?\n'
               r'    if \(!isActiveClientEntity\(entity\)\)\r?\n    \{\r?\n.*?PROBE125\r?\n        delete entity;\r?\n    \}\r?\n\}',
               ('static void deleteWorldOwnedEntity(Entity *entity)' + nl + '{' + nl +
                '    if (!isActiveClientEntity(entity))' + nl + '        delete entity;' + nl + '}'), s, flags=re.S)
    lines = s.split(nl)
    lines = [l for l in lines if 'PROBE125' not in l]
    s = nl.join(lines)
    open(p, 'w', encoding='utf-8', newline='').write(s)
    print(rel, s.count('probe125'))
