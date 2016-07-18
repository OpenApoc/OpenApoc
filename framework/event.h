#pragma once

#include "forms/forms_enums.h"
#include "library/sp.h"
#include "library/strings.h"
// FIXME: Remove SDL headers - we currently use SDL types directly in input events
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>

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
	EVENT_TEXT_INPUT,
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

typedef struct FRAMEWORK_TEXT_EVENT
{
	UString Input;
} FRAMEWORK_TEXT_EVENT;

typedef struct FRAMEWORK_FORMS_EVENT
{
	sp<Control> RaisedBy;
	FormEventType EventFlag;
	FRAMEWORK_MOUSE_EVENT MouseInfo;
	FRAMEWORK_KEYBOARD_EVENT KeyInfo;
	FRAMEWORK_TEXT_EVENT Input;
} FRAMEWORK_FORMS_EVENT;

struct FRAMEWORK_USER_EVENT
{
	UString ID;
	sp<void> data;
	template <typename T> sp<T> dataAs() { return std::static_pointer_cast<T>(this->data); }
};

/*
     Class: Event
     Provides data regarding events that occur within the system
*/
class Event
{
  protected:
	Event(EventTypes type);
	EventTypes eventType;

  public:
	bool Handled;

	EventTypes Type() const;

	FRAMEWORK_DISPLAY_EVENT &Display();
	FRAMEWORK_JOYSTICK_EVENT &Joystick();
	FRAMEWORK_KEYBOARD_EVENT &Keyboard();
	FRAMEWORK_MOUSE_EVENT &Mouse();
	FRAMEWORK_FINGER_EVENT &Finger();
	FRAMEWORK_TIMER_EVENT &Timer();
	FRAMEWORK_FORMS_EVENT &Forms();
	FRAMEWORK_TEXT_EVENT &Text();
	FRAMEWORK_USER_EVENT &User();

	const FRAMEWORK_DISPLAY_EVENT &Display() const;
	const FRAMEWORK_JOYSTICK_EVENT &Joystick() const;
	const FRAMEWORK_KEYBOARD_EVENT &Keyboard() const;
	const FRAMEWORK_MOUSE_EVENT &Mouse() const;
	const FRAMEWORK_FINGER_EVENT &Finger() const;
	const FRAMEWORK_TIMER_EVENT &Timer() const;
	const FRAMEWORK_FORMS_EVENT &Forms() const;
	const FRAMEWORK_TEXT_EVENT &Text() const;
	const FRAMEWORK_USER_EVENT &User() const;

	virtual ~Event() = default;
};

class DisplayEvent : public Event
{
  private:
	FRAMEWORK_DISPLAY_EVENT Data;
	friend class Event;

  public:
	DisplayEvent(EventTypes type);
	~DisplayEvent() override = default;
};

class JoystickEvent : public Event
{
  private:
	FRAMEWORK_JOYSTICK_EVENT Data;
	friend class Event;

  public:
	JoystickEvent(EventTypes type);
	~JoystickEvent() override = default;
};

class KeyboardEvent : public Event
{
  private:
	FRAMEWORK_KEYBOARD_EVENT Data;
	friend class Event;

  public:
	KeyboardEvent(EventTypes type);
	~KeyboardEvent() override = default;
};

class MouseEvent : public Event
{
  private:
	FRAMEWORK_MOUSE_EVENT Data;
	friend class Event;

  public:
	MouseEvent(EventTypes type);
	~MouseEvent() override = default;
};

class FingerEvent : public Event
{
  private:
	FRAMEWORK_FINGER_EVENT Data;
	friend class Event;

  public:
	FingerEvent(EventTypes type);
	~FingerEvent() override = default;
};

class TimerEvent : public Event
{
  private:
	FRAMEWORK_TIMER_EVENT Data;
	friend class Event;

  public:
	TimerEvent(EventTypes type);
	~TimerEvent() override = default;
};

class FormsEvent : public Event
{
  private:
	FRAMEWORK_FORMS_EVENT Data;
	friend class Event;

  public:
	FormsEvent();
	~FormsEvent() override = default;
};

class TextEvent : public Event
{
  private:
	FRAMEWORK_TEXT_EVENT Data;
	friend class Event;

  public:
	TextEvent();
	~TextEvent() override = default;
};

class UserEvent : public Event
{
  private:
	FRAMEWORK_USER_EVENT Data;
	friend class Event;

  public:
	UserEvent(const UString &id, sp<void> data = nullptr);
	~UserEvent() override = default;
};

}; // namespace OpenApoc
