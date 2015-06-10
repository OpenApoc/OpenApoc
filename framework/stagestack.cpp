
#include "framework/stagestack.h"

namespace OpenApoc {

void StageStack::Push(std::shared_ptr<Stage> newStage)
{

	// Pause any current stage
	if(this->Current())
		this->Current()->Pause();

	this->Stack.push_back(newStage);
	newStage->Begin();
}

std::shared_ptr<Stage> StageStack::Pop()
{
	std::shared_ptr<Stage> result = this->Current();

	if (result)
	{
		result->Finish();
		Stack.pop_back();
	}

	// If there's still an item on the stack, resume it
	if( this->Current() )
		this->Current()->Resume();

	return result;
}

std::shared_ptr<Stage> StageStack::Current()
{
	if (this->Stack.empty())
		return nullptr;
	else
		return this->Stack.back();
}

std::shared_ptr<Stage> StageStack::Previous()
{
	return Previous(Current());
}

std::shared_ptr<Stage> StageStack::Previous(std::shared_ptr<Stage> From)
{
	if (!this->Stack.empty())
	{
		for(unsigned int idx = 1; idx < this->Stack.size(); idx++ )
		{
			if( this->Stack.at(idx) == From )
			{
				return this->Stack[idx - 1];
			}
		}
	}
	return nullptr;
}

bool StageStack::IsEmpty()
{
	return this->Stack.empty();
}

void StageStack::Clear()
{
	while (!this->IsEmpty())
		this->Pop();
}

}; //namespace OpenApoc
