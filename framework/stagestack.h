
#pragma once

#include "stage.h"

// Constant: MAX_STACK_SIZE
// Maximum number of stages that can be held in the game loop
#define MAX_STACK_SIZE	12

/*
	Class: StageStack
	Used internally by the framework for keeping track of stages
*/
class StageStack
{
  private:
    int StackIndex;
    Stage* Stack[MAX_STACK_SIZE + 1]; 

  public:

		/*
			Constructor: StageStack
			Sets up the defaults of the stage stack
		*/
    StageStack();

		/*
			Function: Push
			Sets up the defaults of the stage stack
			Parameters:
				newStage - This is the <Stage> object to be put on the top of the stack (make active)
			Returns:
				*Integer* Stack index of the stage
		*/
    int Push( Stage* newStage );

		/*
			Function: Pop
			Removes the top (active) <Stage> from the stack
			Returns:
				*Stage Pointer* Stage object that was popped. Useful for preventing memory leaks
			Example:
				> delete StageStack->Pop();
		*/
    Stage* Pop();

		/*
			Function: Remove
			Removes a <Stage> from anywhere in the stack
			Parameters:
				Index - Index of the stage you want to retrieve
			Returns:
				*Stage Pointer* Stage object that was removed. Useful for preventing memory leaks
		*/
    Stage* Remove( int Index );

		/*
			Function: Remove
			Removes a <Stage> from anywhere in the stack
			Parameters:
				RemoveStage - Pointer to a <Stage> class to remove
			Returns:
				*Stage Pointer* Stage object that was removed. Useful for preventing memory leaks
		*/
    Stage* Remove( Stage* RemoveStage );

		/*
			Function: Current
			Returns a pointer to the current active stage
			Returns:
				*Stage Pointer* Current <Stage>
		*/
    Stage* Current();

		/*
			Function: GetStackIndex
			Returns the index of the top of the stack. Useful for checking with <MAX_STACK_SIZE> if you use a lot of stages
			Returns:
				*Integer*
		*/
    int GetStackIndex();

		/*
			Function: Item
			Returns a pointer to the stage at the chosen index
			Parameters:
				Index - Index of the stage you want to retrieve
			Returns:
				*Stage Pointer* Current <Stage>
		*/
    Stage* Item( int Index );

		/*
			Function: Previous
			Returns a pointer to the previous non-transitional stage on the stack
			Returns:
				*Stage Pointer* Returns 0 if no previous <Stage> is available
		*/
		Stage* Previous();

		/*
			Function: Previous
			Returns a pointer to the previous non-transitional stage on the stack
			Parameters:
				CheckStage - Pointer to the <Stage> you want the Stage before of
			Returns:
				*Stage Pointer* Returns 0 if no previous <Stage> is available
		*/
		Stage* Previous( Stage* CheckStage );

		/*
			Function: Previous
			Returns a pointer to the previous stage on the stack (Transitional stages are optional)
			Parameters:
				CheckStage - Pointer to the <Stage> you want the Stage before of
				IncludeTransitions - Include transition stages
			Returns:
				*Stage Pointer* Returns 0 if no previous <Stage> is available
		*/
		Stage* Previous( Stage* CheckStage, bool IncludeTransitions );

		/*
			Function: IsEmpty
			Indicates if there are no stages on the stack. The <Framework> will quit the program if no stages are on the stack.
			However, you can <Pop> the last <Stage>, and <Push> a new <Stage> before returning control to the <Framework>, and things
			will continue.
			Returns:
				*Stage Pointer* Returns 0 if no previous <Stage> is available
		*/
		bool IsEmpty();

};
