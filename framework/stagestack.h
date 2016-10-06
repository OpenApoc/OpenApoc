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
	void push(sp<Stage> newStage);

	/*
	    Function: Pop
	    Removes the top (active) <Stage> from the stack
	    Returns:
	        *Stage Pointer* Stage object that was popped. Useful for preventing memory leaks
	    Example:
	        > delete StageStack->Pop();
	*/
	sp<Stage> pop();

	/*
	    Function: Current
	    Returns a pointer to the current active stage
	    Returns:
	        *Stage Pointer* Current <Stage>
	*/
	sp<Stage> current();

	/*
	    Function: Previous
	    Returns a pointer to the previous stage to the active stage
	    Returns:
	        *Stage Pointer* Current <Stage>
	*/
	sp<Stage> previous();

	/*
	    Function: Previous
	    Returns a pointer to the previous stage to a given stage
	    Returns:
	        *Stage Pointer* Current <Stage>
	*/
	sp<Stage> previous(sp<Stage> From);

	bool isEmpty();
	void clear();
};

}; // namespace OpenApoc
