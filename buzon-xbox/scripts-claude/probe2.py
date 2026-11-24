import sys
p = sys.argv[1] + '/src/net/minecraft/src/World.cpp'
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'


def rep(a, b):
    global s
    a = a.replace('\n', nl)
    b = b.replace('\n', nl)
    assert s.count(a) == 1, a
    s = s.replace(a, b)


rep('''void World::detachEntityForWorldChange(Entity *entity)
{
    if (entity == nullptr)
        return;
''', '''void World::detachEntityForWorldChange(Entity *entity)
{
    if (entity == nullptr)
        return;
    MC_LOG_WARN("probe125", "detach %p from world %p loaded=%u players=%u\\n", (void *)entity, (void *)this, (unsigned)loadedEntityList.size(), (unsigned)playerEntities.size()); // PROBE125
''')
rep('''        if (entity->isPlayer())
        {
            EntityPlayer* entityPlayer = static_cast<EntityPlayer*>(entity);
            playerEntities.push_back(entityPlayer);''', '''        if (entity->isPlayer())
        {
            EntityPlayer* entityPlayer = static_cast<EntityPlayer*>(entity);
            MC_LOG_WARN("probe125", "join player %p into world %p id=%d\\n", (void *)entity, (void *)this, entity->entityId); // PROBE125
            playerEntities.push_back(entityPlayer);''')
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')
