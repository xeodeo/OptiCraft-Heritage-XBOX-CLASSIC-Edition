import sys
root = sys.argv[1] + '/src/'


def edit(p, pairs):
    s = open(root + p, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    for a, b in pairs:
        a = a.replace('\n', nl)
        b = b.replace('\n', nl)
        assert s.count(a) == 1, (p, a)
        s = s.replace(a, b)
    open(root + p, 'w', encoding='utf-8', newline='').write(s)


edit('net/minecraft/src/World.h', [('''	void detachEntityForWorldChange(Entity *entity);''',
'''	void detachEntityForWorldChange(Entity *entity);
	// Drops a pointer from every world-owned entity list WITHOUT touching the object
	// (it may already be freed by its other world). Used before deleting abandoned
	// worlds so a local player that crossed a dimension change is deleted only once.
	void forgetEntityPointer(Entity *entity);''')])

edit('net/minecraft/src/World.cpp', [('''void World::detachEntityForWorldChange(Entity *entity)
{''', '''void World::forgetEntityPointer(Entity *entity)
{
    if (entity == nullptr)
        return;
    loadedEntityList.erase(std::remove(loadedEntityList.begin(), loadedEntityList.end(), entity), loadedEntityList.end());
    untrackLoadedEntityPointer(entity);
    entityCountsDirty = true;
    unloadedEntityList.erase(std::remove(unloadedEntityList.begin(), unloadedEntityList.end(), entity), unloadedEntityList.end());
    weatherEffects.erase(std::remove(weatherEffects.begin(), weatherEffects.end(), entity), weatherEffects.end());
    playerEntities.erase(std::remove_if(playerEntities.begin(), playerEntities.end(),
        [entity](EntityPlayer *p) { return static_cast<void *>(p) == static_cast<void *>(entity); }), playerEntities.end());
}

void World::detachEntityForWorldChange(Entity *entity)
{''')])

edit('client/Minecraft.h', [('''    std::vector<World *> worldsToDelete;''',
'''    // Local players that respawn() replaced (multiplayer dimension changes). Their
    // pointer can still sit in an abandoned world's lists; forgotten there before
    // that world is deleted, so only the current world deletes them (crash 23-11 on
    // a 1.12.2 server: two dimension changes on respawn -> double delete).
    std::vector<Entity *> retiredLocalPlayers;
    std::vector<World *> worldsToDelete;''')])

edit('client/Minecraft.cpp', [
    ('''    EntityPlayerSP *oldPlayer = thePlayer;
    if (thePlayer != nullptr)
    {
        j = thePlayer->entityId;
        theWorld->setEntityDead(thePlayer);
    }''', '''    EntityPlayerSP *oldPlayer = thePlayer;
    if (thePlayer != nullptr)
    {
        j = thePlayer->entityId;
        theWorld->setEntityDead(thePlayer);
        if (theWorld->multiplayerWorld)
            retiredLocalPlayers.push_back(thePlayer);
    }'''),
    ('''        if (thePlayer != nullptr && theWorld != nullptr)
        {
            for (World *w : worldsToDelete)
                if (w != nullptr && w != theWorld)
                    w->detachEntityForWorldChange(thePlayer);''', '''        // Players replaced by respawn() belong to the current world only (it deletes
        // them once, dead); drop any pointer an abandoned world still keeps.
        for (World *w : worldsToDelete)
            if (w != nullptr && w != theWorld)
                for (Entity *retired : retiredLocalPlayers)
                    w->forgetEntityPointer(retired);
        retiredLocalPlayers.clear();
        if (thePlayer != nullptr && theWorld != nullptr)
        {
            for (World *w : worldsToDelete)
                if (w != nullptr && w != theWorld)
                    w->detachEntityForWorldChange(thePlayer);'''),
])
print('ok')
