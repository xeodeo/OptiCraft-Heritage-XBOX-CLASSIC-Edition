import sys
p = sys.argv[1] + '/src/net/minecraft/src/World.cpp'
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'
a = '''static void deleteWorldOwnedEntity(Entity *entity)
{
    if (!isActiveClientEntity(entity))
        delete entity;
}'''.replace('\n', nl)
b = '''static void deleteWorldOwnedEntity(Entity *entity)
{
    if (!isActiveClientEntity(entity))
    {
        MC_LOG_WARN("probe125", "del %p cls=%d id=%d\\n", (void *)entity, entity != nullptr ? entity->getEntityClassID() : -1, entity != nullptr ? entity->entityId : -1); // PROBE125
        delete entity;
    }
}'''.replace('\n', nl)
assert s.count(a) == 1
s = s.replace(a, b)
a2 = 'World::~World()' + nl + '{' + nl
assert s.count(a2) == 1
s = s.replace(a2, a2 + '    MC_LOG_WARN("probe125", "~World %p\\n", (void *)this); // PROBE125' + nl)
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')
