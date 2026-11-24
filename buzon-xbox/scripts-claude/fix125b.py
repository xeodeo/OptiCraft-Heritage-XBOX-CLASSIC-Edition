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


edit('World.h', [
    ('	void detachEntityForWorldChange(Entity *entity);', '	virtual void detachEntityForWorldChange(Entity *entity);'),
    ('	void forgetEntityPointer(Entity *entity);', '	virtual void forgetEntityPointer(Entity *entity);'),
])
edit('WorldClient.h', [('	void setEntityDead(Entity *entity) override;',
'''	void setEntityDead(Entity *entity) override;
	// The local player crossing a dimension change must also leave knownEntities:
	// ~WorldClient hands every known entity back to World for deletion.
	void detachEntityForWorldChange(Entity *entity) override;
	void forgetEntityPointer(Entity *entity) override;''')])
edit('WorldClient.cpp', [('''void WorldClient::setEntityDead(Entity *entity)
{''', '''void WorldClient::detachEntityForWorldChange(Entity *entity)
{
	if (entity == nullptr)
		return;
	knownEntities.remove(entity);
	entitySpawnQueue.remove(entity);
	World::detachEntityForWorldChange(entity);
}

void WorldClient::forgetEntityPointer(Entity *entity)
{
	if (entity == nullptr)
		return;
	// Pointer-only: the object may already be freed by the world that owns it now.
	knownEntities.remove(entity);
	entitySpawnQueue.remove(entity);
	World::forgetEntityPointer(entity);
}

void WorldClient::setEntityDead(Entity *entity)
{''')])
print('ok')
