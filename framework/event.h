
#pragma once

#include "includes.h"

#include "forms/forms_enums.h"

namespace OpenApoc
{

class Control;

enum EventTypes
{
	EVENT_WINDOW_ACTIVATE,
	EVENT_WINDOW_DEACTIVATE,
	EVENT_WINDOW_RESIZE,
	EVENT_WINDOW_MANAGER,
	EVENT_WINDOW_CLOSED,
	EVENT_KEY_DOWN,
	EVENT_KEY_PRESS,
	EVENT_KEY_UP,
	EVENT_MOUSE_DOWN,
	EVENT_MOUSE_UP,
	EVENT_MOUSE_MOVE,
	EVENT_FINGER_DOWN,
	EVENT_FINGER_UP,
	EVENT_FINGER_MOVE,
	EVENT_JOYSTICK_AXIS,
	EVENT_JOYSTICK_HAT,
	EVENT_JOYSTICK_BALL,
	EVENT_JOYSTICK_BUTTON_DOWN,
	EVENT_JOYSTICK_BUTTON_UP,
	EVENT_TIMER_TICK,
	EVENT_AUDIO_STREAM_FINISHED,
	EVENT_FORM_INTERACTION,
	EVENT_USER,
	EVENT_END_OF_FRAME,
	EVENT_UNDEFINED
};

typedef struct FRAMEWORK_DISPLAY_EVENT
{
	bool Active;
	int X;
	int Y;
	int Width;
	int Height;
} FRAMEWORK_DISPLAY_EVENT;

typedef struct FRAMEWORK_JOYSTICK_EVENT
{
	int ID;
	int Stick;
	int Axis;
	float Position;
	int Button;
} FRAMEWORK_JOYSTICK_EVENT;

typedef struct FRAMEWORK_MOUSE_EVENT
{
	int X;
	int Y;
	int WheelVertical;
	int WheelHorizontal;
	int DeltaX;
	int DeltaY;
	int Button;
} FRAMEWORK_MOUSE_EVENT;

typedef struct FRAMEWORK_FINGER_EVENT
{
	// Touch coordinates and deltas
	int X;
	int Y;
	int DeltaX;
	int DeltaY;
	// Touch ID (system-specified)
	int Id;
	// Should this be considered a "primary" touch? (first finger?)
	bool IsPrimary;
} FRAMEWORK_FINGER_EVENT;

typedef struct FRAMEWORK_KEYBOARD_EVENT
{
	int KeyCode;
	int UniChar;
	unsigned int Modifiers;
} FRAMEWORK_KEYBOARD_EVENT;

typedef struct FRAMEWORK_TIMER_EVENT
{
	void *TimerObject;
} FRAMEWORK_TIMER_EVENT;

typedef struct FRAMEWORK_FORMS_EVENT
{
	Control *RaisedBy;
	FormEventType EventFlag;
	FRAMEWORK_MOUSE_EVENT MouseInfo;
	FRAMEWORK_KEYBOARD_EVENT KeyInfo;
} FRAMEWORK_FORMS_EVENT;

typedef union EventData
{
	FRAMEWORK_DISPLAY_EVENT Display;
	FRAMEWORK_JOYSTICK_EVENT Joystick;
	FRAMEWORK_KEYBOARD_EVENT Keyboard;
	FRAMEWORK_MOUSE_EVENT Mouse;
	FRAMEWORK_FINGER_EVENT Finger;
	FRAMEWORK_TIMER_EVENT Timer;
	FRAMEWORK_FORMS_EVENT Forms;
} EventData;

/*
     Class: Event
     Provides data regarding events that occur within the system
*/
class Event
{
  public:
	bool Handled;
	EventTypes Type;
	EventData Data;

	/*
	    Constructor: Event
	    Defaults the <Type> to Undefined
	*/
	Event();

	/*
	    Destructor: ~Event
	    For network packets, it calls enet's packet delete.
	    For download packets, url and the data are deleted (assumption is that the program will have
	   processed the data)
	*/
	~Event();
};
}; // namespace OpenApoc
