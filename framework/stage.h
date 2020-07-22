#pragma once

#include "framework/logger.h"
#include "library/sp.h"

namespace OpenApoc
{

class Stage;
class Surface;
class Event;

class StageCmd
{
  public:
	enum class Command
	{
		CONTINUE,   // Continue as normal
		REPLACE,    // Remove the current head and replace with nextStage
		REPLACEALL, // Remove all current Stages and then push nextStage as the new head
		POP,        // Remove the current head
		PUSH,       // Push nextStage onto the top of the stack
		QUIT,       // Clear the stage stack, exiting the program
	};

	Command cmd;

	sp<Stage> nextStage;

	StageCmd() : cmd(Command::CONTINUE) {}
	StageCmd(Command cmd, sp<Stage> nextStage = nullptr) : cmd(cmd), nextStage(nextStage)
	{
		switch (cmd)
		{
			case Command::REPLACE:
			case Command::REPLACEALL:
			case Command::PUSH:
				LogAssert(nextStage != nullptr);
				break;
			case Command::CONTINUE:
			case Command::POP:
			case Command::QUIT:
				LogAssert(nextStage == nullptr);
				break;
		}
	}
};

/*
    Class: Stage
    You must inherit this in any game "screens", as it provides the framework's functionality
*/

class Stage : public std::enable_shared_from_this<Stage>
{
  public:
	Stage() {}
	/*
	    Function: Begin
	    This function is called just before the stage becomes the *active* stage.
	    You can use it instead of the constructor if you need to reset variables when returning to
	   this stage from a further stage
	*/
	virtual void begin() = 0;

	/*
	    Function: Pause
	    This function is called when a new stage is being made active. Use this to stop any game
	   timers, or music etc.
	*/
	virtual void pause() = 0;

	/*
	    Function: Resume
	    This function is called when all later stages have popped off the stack, and this becomes
	   the current stage again
	*/
	virtual void resume() = 0;

	/*
	    Function: Finish
	    This function is called when the stage is popped off the stack, and will no longer be used.
	*/
	virtual void finish() = 0;

	/*
	    Function: EventOccurred
	    This function is called only on the active stage.
	    All events generated within the program are provided to the stage for any additional
	   processing
	    Parameters:
	        e - Event object detailing the event that has occurred
	*/
	virtual void eventOccurred(Event *e) = 0;

	/*
	    Function: Update
	    Called for each game tick based upon the <FRAMES_PER_SECOND>.
	*/
	virtual void update() = 0;

	/*
	    Function: Render
	    This function is called when the screen needs to be redrawn
	*/
	virtual void render() = 0;

	/*
	    Function: IsTransition
	    This function indicates whether the stage is a transition and should be deleted when it's
	   finished
	    Returns:
	        *Boolean* Indicates if stage should be considered as a transition stage
	*/
	virtual bool isTransition() = 0;

	/* Need a virtual destructor to correctly call any subclass destructors */
	virtual ~Stage() = default;
};

}; // namespace OpenApoc
