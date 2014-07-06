
#include "stagestack.h"

StageStack::StageStack()
{
  StackIndex = -1;
}

int StageStack::Push(Stage* newStage)
{
  if( StackIndex == MAX_STACK_SIZE - 1 )
    return -1;

  // Pause any current stage
  if( StackIndex != -1 )
    this->Current()->Pause();

  StackIndex++;
  Stack[StackIndex] = newStage;
  Stack[StackIndex]->Begin();

  return StackIndex;
}

Stage* StageStack::Pop()
{
  Stage* result;

  // Remove stage from stack
  result = this->Current();
  result->Finish();
  Stack[StackIndex] = 0;
  StackIndex--;

  // If there's still an item on the stack, resume it
  if( StackIndex != -1 )
    this->Current()->Resume();

  return result;
}

Stage* StageStack::Remove( int Index )
{
  Stage* result;

  if( StackIndex < Index )
	{
		return 0;	// No stage at this location
	}
	result = Stack[Index];
	for( int i = Index; i < StackIndex; i++ )
	{
		Stack[i] = Stack[i + 1];
	}
	StackIndex--;

  return result;
}

Stage* StageStack::Remove( Stage* RemoveStage )
{
  for( int i = StackIndex; i >= 0; i-- )
	{
		if( Stack[i] == RemoveStage )
		{
			for( int j = i; j < StackIndex; j++ )
			{
				Stack[j] = Stack[j + 1];
			}
			StackIndex--;
			return RemoveStage;
		}
	}
	return 0;
}

Stage* StageStack::Current()
{
  if( StackIndex == -1 )
    return 0;

  return Stack[StackIndex];
}

int StageStack::GetStackIndex()
{
  return StackIndex;
}

Stage* StageStack::Item( int Index )
{
  return Stack[Index];
}

Stage* StageStack::Previous()
{
  if( StackIndex <= 0 )
    return 0;
	return Previous( Current(), false );
}

Stage* StageStack::Previous( Stage* CheckStage )
{
	return Previous( CheckStage, false );
}

Stage* StageStack::Previous( Stage* CheckStage, bool IncludeTransitions )
{
  if( StackIndex < 0 )
    return 0;

	for( int i = StackIndex; i >= 0; i-- )
	{
		if( Stack[i] == CheckStage )
		{
			if( i == 0 )
			{
				return 0;
			}
			if( IncludeTransitions )
			{
				return Stack[i-1];
			} else {
				for( int j = i - 1; j >= 0; j-- )
				{
					if( !Stack[j]->IsTransition() )
					{
						return Stack[j];
					}
				}
			}
		}
	}

  return 0;
}

bool StageStack::IsEmpty()
{
	return (StackIndex < 0);
}
