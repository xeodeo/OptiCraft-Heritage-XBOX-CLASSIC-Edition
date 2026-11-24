import sys
p = sys.argv[1] + '/src/net/minecraft/src/World.cpp'
s = open(p, encoding='utf-8', newline='').read()
a = 'static void deleteWorldOwnedEntity(Entity *entity)'
assert s.count(a) == 1
s = s.replace(a, '#include <intrin.h>\n__declspec(noinline) static void deleteWorldOwnedEntity(Entity *entity)', 1)
a2 = 'MC_LOG_WARN("probe125", "del %p cls=%d id=%d\\n", (void *)entity,'
assert s.count(a2) == 1, 'probe3 line not found'
s = s.replace(a2, 'MC_LOG_WARN("probe125", "del %p from %p cls=%d id=%d\\n", (void *)entity, _ReturnAddress(),')
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')
