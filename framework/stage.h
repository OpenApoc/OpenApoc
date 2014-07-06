
#pragma once

#include "event.h"

/*
	Class: Stage
	You must inherit this in any game "screens", as it provides the framework's functionality
*/
class Stage
{
  public:
		/*
			Function: Begin
			This function is called just before the stage becomes the *active* stage.
			You can use it instead of the constructor if you need to reset variables when returning to this stage from a further stage
    */
    virtual void Begin() = 0;

		/*
			Function: Pause
			This function is called when a new stage is being made active. Use this to stop any game timers, or music etc.
    */
    virtual void Pause() = 0;

		/*
			Function: Resume
			This function is called when all later stages have popped off the stack, and this becomes the current stage again
    */
    virtual void Resume() = 0;

		/*
			Function: Finish
			This function is called when the stage is popped off the stack, and will no longer be used.
    */
    virtual void Finish() = 0;

		/*
			Function: EventOccurred
			This function is called only on the active stage.
			All events generated within the program are provided to the stage for any additional processing
			Parameters: 
				e - Event object detailing the event that has occured
    */
    virtual void EventOccurred(Event *e) = 0;

		/*
			Function: Update
			Called for each game tick based upon the <FRAMES_PER_SECOND>.
    */
    virtual void Update() = 0;

		/*
			Function: Render
			This function is called when the screen needs to be redrawn
    */
    virtual void Render() = 0;

		/*
			Function: IsTransition
			This function indicates whether the stage is a transition and should be deleted when it's finished
			Returns:
				*Boolean* Indicates if stage should be considered as a transition stage
    */
    virtual bool IsTransition() = 0;
};
