
#pragma once
#include "library/sp.h"

#include "stage.h"

#include <vector>

namespace OpenApoc
{

/*
    Class: StageStack
    Used internally by the framework for keeping track of stages
*/
class StageStack
{
  private:
	std::vector<sp<Stage>> Stack;

  public:
	/*
	    Function: Push
	    Sets up the defaults of the stage stack
	    Parameters:
	        newStage - This is the <Stage> object to be put on the top of the stack (make active)
	    Returns:
	        *Integer* Stack index of the stage
	*/
	void Push(sp<Stage> newStage);

	/*
	    Function: Pop
	    Removes the top (active) <Stage> from the stack
	    Returns:
	        *Stage Pointer* Stage object that was popped. Useful for preventing memory leaks
	    Example:
	        > delete StageStack->Pop();
	*/
	sp<Stage> Pop();

	/*
	    Function: Current
	    Returns a pointer to the current active stage
	    Returns:
	        *Stage Pointer* Current <Stage>
	*/
	sp<Stage> Current();

	/*
	    Function: Previous
	    Returns a pointer to the previous stage to the active stage
	    Returns:
	        *Stage Pointer* Current <Stage>
	*/
	sp<Stage> Previous();

	/*
	    Function: Previous
	    Returns a pointer to the previous stage to a given stage
	    Returns:
	        *Stage Pointer* Current <Stage>
	*/
	sp<Stage> Previous(sp<Stage> From);

	bool IsEmpty();
	void Clear();
};

}; // namespace OpenApoc
