
#pragma once

#include "stage.h"

#include <stack>

namespace OpenApoc {

/*
	Class: StageStack
	Used internally by the framework for keeping track of stages
*/
class StageStack
{
	private:
	std::stack<std::shared_ptr<Stage> > Stack;

	public:

		/*
			Function: Push
			Sets up the defaults of the stage stack
			Parameters:
				newStage - This is the <Stage> object to be put on the top of the stack (make active)
			Returns:
				*Integer* Stack index of the stage
		*/
		void Push( std::shared_ptr<Stage> newStage );

		/*
			Function: Pop
			Removes the top (active) <Stage> from the stack
			Returns:
				*Stage Pointer* Stage object that was popped. Useful for preventing memory leaks
			Example:
				> delete StageStack->Pop();
		*/
		std::shared_ptr<Stage> Pop();

		/*
			Function: Current
			Returns a pointer to the current active stage
			Returns:
				*Stage Pointer* Current <Stage>
		*/
		std::shared_ptr<Stage> Current();

		bool IsEmpty();
		void Clear();

};

}; //namespace OpenApoc
