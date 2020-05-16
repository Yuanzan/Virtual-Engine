#include "RuntimeStatic.h"
bool RuntimeStaticBase::constructed = false;
RuntimeStaticBase* RuntimeStaticBase::first = nullptr;
void RuntimeStaticBase::ConstructAll()
{
	if (constructed) return;
	constructed = true;
	RuntimeStaticBase* b = first;
	while (b != nullptr)
	{
		b->constructor(b->dataPtr);
		b = b->next;
	}
}

void RuntimeStaticBase::DestructAll()
{
	if (!constructed) return;
	constructed = false;
	RuntimeStaticBase* b = first;
	while (b != nullptr)
	{
		b->destructor(b->dataPtr);
		b = b->next;
	}
}