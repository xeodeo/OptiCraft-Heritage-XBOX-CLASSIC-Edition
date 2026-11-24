import sys
d = sys.argv[1] + '/src/net/minecraft/src/'


def edit(p, pairs):
    s = open(d + p, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    for a, b in pairs:
        a = a.replace('\n', nl)
        b = b.replace('\n', nl)
        assert s.count(a) == 1, (p, a)
        s = s.replace(a, b)
    open(d + p, 'w', encoding='utf-8', newline='').write(s)


edit('World.cpp', [('''    if (chunkProvider != nullptr)
        chunkProvider->removeEntityFromLoadedChunks(entity);
    entity->addedToChunk = false;
    deleteWorldOwnedEntity(entity);
}''', '''    if (chunkProvider != nullptr)
        chunkProvider->removeEntityFromLoadedChunks(entity);
    entity->addedToChunk = false;
    // Never leave a freed player in playerEntities: closeContainersUsing() and the
    // sleep checks walk it (crash 23-11, 1.5.2 on a 1.12.2 server, chunk unload).
    if (entity->isPlayer())
        playerEntities.erase(std::remove(playerEntities.begin(), playerEntities.end(), static_cast<EntityPlayer *>(entity)), playerEntities.end());
    deleteWorldOwnedEntity(entity);
}''')])

edit('WorldClient.cpp', [('''#if PLATFORM_PS2
			if (entity->isPlayer())
				playerEntities.erase(std::remove(playerEntities.begin(), playerEntities.end(), static_cast<EntityPlayer *>(entity)), playerEntities.end());
			MC_LOG_DEBUG("net.entity", "chunk-detach id=%d chunk=%d,%d\\n", entity->entityId, entity->chunkCoordX, entity->chunkCoordZ);
#endif''', '''			// A remote player waiting for its chunk is not in loadedEntityList, so it
			// must not stay in playerEntities either (all platforms, not just PS2).
			if (entity->isPlayer())
				playerEntities.erase(std::remove(playerEntities.begin(), playerEntities.end(), static_cast<EntityPlayer *>(entity)), playerEntities.end());
#if PLATFORM_PS2
			MC_LOG_DEBUG("net.entity", "chunk-detach id=%d chunk=%d,%d\\n", entity->entityId, entity->chunkCoordX, entity->chunkCoordZ);
#endif''')])
print('ok')
