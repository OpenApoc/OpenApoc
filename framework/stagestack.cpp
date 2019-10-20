#include "framework/stagestack.h"
#include "library/sp.h"

namespace OpenApoc
{

void StageStack::push(sp<Stage> newStage)
{

	// Pause any current stage
	if (this->current())
		this->current()->pause();

	this->Stack.push_back(newStage);
	newStage->begin();
}

sp<Stage> StageStack::pop()
{
	sp<Stage> result = this->current();

	if (result)
	{
		result->finish();
		Stack.pop_back();
	}

	// If there's still an item on the stack, resume it
	if (this->current())
		this->current()->resume();

	return result;
}

sp<Stage> StageStack::current()
{
	if (this->Stack.empty())
		return nullptr;
	else
		return this->Stack.back();
}

sp<Stage> StageStack::previous() { return previous(current()); }

sp<Stage> StageStack::previous(sp<Stage> From)
{
	if (!this->Stack.empty())
	{
		for (unsigned int idx = 1; idx < this->Stack.size(); idx++)
		{
			if (this->Stack.at(idx) == From)
			{
				return this->Stack[idx - 1];
			}
		}
	}
	return nullptr;
}

bool StageStack::isEmpty() { return this->Stack.empty(); }

void StageStack::clear()
{
	while (!this->isEmpty())
		this->pop();
}

}; // namespace OpenApoc
