#include "EntityAITaskEntry.h"

#include "EntityAIBase.h"

EntityAITaskEntry::EntityAITaskEntry(EntityAITasks *owner, int_t taskPriority, EntityAIBase *taskAction, bool owns) :
	action(taskAction), priority(taskPriority), tasks(owner), executing(false), ownsAction(owns)
{
}

EntityAITaskEntry::~EntityAITaskEntry()
{
	if (ownsAction)
	{
		delete action;
	}
}
