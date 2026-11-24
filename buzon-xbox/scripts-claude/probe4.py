import sys
p = sys.argv[1] + '/src/client/Minecraft.cpp'
s = open(p, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in s else '\n'


def rep(a, b):
    global s
    a = a.replace('\n', nl)
    b = b.replace('\n', nl)
    assert s.count(a) == 1, a
    s = s.replace(a, b)


rep('''            retiredLocalPlayers.push_back(thePlayer);''',
    '''            retiredLocalPlayers.push_back(thePlayer);
        MC_LOG_WARN("probe125", "retire %p in world %p\\n", (void *)thePlayer, (void *)theWorld); // PROBE125''')
rep('''        for (World *w : worldsToDelete)
            if (w != nullptr && w != theWorld)
                for (Entity *retired : retiredLocalPlayers)
                    w->forgetEntityPointer(retired);''',
    '''        MC_LOG_WARN("probe125", "flush worlds=%u retired=%u cur=%p player=%p\\n", (unsigned)worldsToDelete.size(), (unsigned)retiredLocalPlayers.size(), (void *)theWorld, (void *)thePlayer); // PROBE125
        for (World *w : worldsToDelete)
            if (w != nullptr && w != theWorld)
                for (Entity *retired : retiredLocalPlayers)
                    w->forgetEntityPointer(retired);''')
rep('''    EntityPlayerSP *transferredPlayer = entityplayer;
    if (transferredPlayer == nullptr && world != nullptr && world->multiplayerWorld)
        transferredPlayer = thePlayer;''',
    '''    EntityPlayerSP *transferredPlayer = entityplayer;
    if (transferredPlayer == nullptr && world != nullptr && world->multiplayerWorld)
        transferredPlayer = thePlayer;
    MC_LOG_WARN("probe125", "changeWorld old=%p new=%p transferred=%p\\n", (void *)theWorld, (void *)world, (void *)transferredPlayer); // PROBE125''')
open(p, 'w', encoding='utf-8', newline='').write(s)
print('ok')
