#include "EntityAITasks.h"

#include <algorithm>

#include "EntityAIBase.h"
#include "EntityAITaskEntry.h"

EntityAITasks::EntityAITasks() = default;

EntityAITasks::~EntityAITasks() = default;

void EntityAITasks::addTask(int_t priority, EntityAIBase *action, bool ownsAction)
{
	if (action == nullptr)
		return;
	tasksToDo.push_back(std::unique_ptr<EntityAITaskEntry>(new EntityAITaskEntry(this, priority, action, ownsAction)));
}

void EntityAITasks::removeTask(EntityAIBase *action)
{
	if (action == nullptr)
		return;
	for (auto it = tasksToDo.begin(); it != tasksToDo.end(); ++it)
	{
		if ((*it)->action == action)
		{
			EntityAITaskEntry *entry = it->get();
			if (isExecuting(entry))
			{
				action->resetTask();
				removeExecuting(entry);
			}
			tasksToDo.erase(it);
			break;
		}
	}
}

void EntityAITasks::onUpdateTasks(bool evaluateTransitions)
{
	std::vector<EntityAITaskEntry *> startingTasks;
	for (const std::unique_ptr<EntityAITaskEntry> &ownedEntry : tasksToDo)
	{
		EntityAITaskEntry *entry = ownedEntry.get();
		bool executing = isExecuting(entry);
		if (executing)
		{
			// Even a throttled decision tick must retire invalid running tasks.
			// Only the search for NEW work is deferred; otherwise an attack,
			// avoidance or target task can continue for several stale ticks.
			if (!canUse(entry) || !entry->action->continueExecuting())
			{
				entry->action->resetTask();
				removeExecuting(entry);
				executing = false;
			}
		}

		if (evaluateTransitions && !executing && canUse(entry) && entry->action->shouldExecute())
		{
			startingTasks.push_back(entry);
			executingTasks.push_back(entry);
			entry->executing = true;
		}
	}

	for (EntityAITaskEntry *entry : startingTasks)
		entry->action->startExecuting();
	for (EntityAITaskEntry *entry : executingTasks)
		entry->action->updateTask();
}

bool EntityAITasks::canUse(EntityAITaskEntry *entry) const
{
	for (EntityAITaskEntry *other : executingTasks)
	{
		if (other == entry)
			continue;

		if (entry->priority >= other->priority)
		{
			if (!areTasksCompatible(entry, other))
				return false;
		}
		else if (!other->action->isContinuous())
		{
			return false;
		}
	}
	return true;
}

bool EntityAITasks::areTasksCompatible(EntityAITaskEntry *first, EntityAITaskEntry *second) const
{
	return (first->action->getMutexBits() & second->action->getMutexBits()) == 0;
}

bool EntityAITasks::isExecuting(const EntityAITaskEntry *entry) const
{
	return entry != nullptr && entry->executing;
}

void EntityAITasks::removeExecuting(EntityAITaskEntry *entry)
{
	if (entry == nullptr || !entry->executing)
		return;
	auto it = std::find(executingTasks.begin(), executingTasks.end(), entry);
	if (it != executingTasks.end())
		executingTasks.erase(it);
	entry->executing = false;
}
