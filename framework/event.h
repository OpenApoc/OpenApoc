#pragma once

#include "forms/forms_enums.h"
#include "library/sp.h"
#include "library/strings.h"

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
	EVENT_MOUSE_SCROLL,
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
	EVENT_GAME_STATE,
	EVENT_USER,
	EVENT_END_OF_FRAME,
	EVENT_UNDEFINED
};

typedef struct FrameworkDisplayEvent
{
	bool Active;
	int X;
	int Y;
	int Width;
	int Height;
} FrameworkDisplayEvent;

typedef struct FrameworkJoystickEvent
{
	int ID;
	int Stick;
	int Axis;
	float Position;
	int Button;
} FrameworkJoystickEvent;

typedef struct FrameworkMouseEvent
{
	int X;
	int Y;
	int WheelVertical;
	int WheelHorizontal;
	int DeltaX;
	int DeltaY;
	int Button;
} FrameworkMouseEvent;

typedef struct FrameworkFingerEvent
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
} FrameworkFingerEvent;

typedef struct FrameworkKeyboardEvent
{
	int KeyCode;
	int ScanCode;
	unsigned int Modifiers;
} FrameworkKeyboardEvent;

typedef struct FrameworkTimerEvent
{
	void *TimerObject;
} FrameworkTimerEvent;

typedef struct FrameworkTextEvent
{
	UString Input;
} FrameworkTextEvent;

typedef struct FrameworkFormsEvent
{
	sp<Control> RaisedBy;
	FormEventType EventFlag;
	FrameworkMouseEvent MouseInfo;
	FrameworkKeyboardEvent KeyInfo;
	FrameworkTextEvent Input;
} FrameworkFormsEvent;

struct FrameworkUserEvent
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
  public:
	enum class MouseButton
	{
		Left = 1,
		Middle = 2,
		Right = 3,
		Back = 4,
		Forward = 5
	};

	static bool isPressed(int mask, MouseButton button);

  protected:
	Event(EventTypes type);
	EventTypes eventType;

  public:
	bool Handled;

	EventTypes type() const;

	FrameworkDisplayEvent &display();
	FrameworkJoystickEvent &joystick();
	FrameworkKeyboardEvent &keyboard();
	FrameworkMouseEvent &mouse();
	FrameworkFingerEvent &finger();
	FrameworkTimerEvent &timer();
	FrameworkFormsEvent &forms();
	FrameworkTextEvent &text();
	FrameworkUserEvent &user();

	const FrameworkDisplayEvent &display() const;
	const FrameworkJoystickEvent &joystick() const;
	const FrameworkKeyboardEvent &keyboard() const;
	const FrameworkMouseEvent &mouse() const;
	const FrameworkFingerEvent &finger() const;
	const FrameworkTimerEvent &timer() const;
	const FrameworkFormsEvent &forms() const;
	const FrameworkTextEvent &text() const;
	const FrameworkUserEvent &user() const;

	virtual ~Event() = default;
};

class DisplayEvent : public Event
{
  private:
	FrameworkDisplayEvent Data;
	friend class Event;

  public:
	DisplayEvent(EventTypes type);
	~DisplayEvent() override = default;
};

class JoystickEvent : public Event
{
  private:
	FrameworkJoystickEvent Data;
	friend class Event;

  public:
	JoystickEvent(EventTypes type);
	~JoystickEvent() override = default;
};

class KeyboardEvent : public Event
{
  private:
	FrameworkKeyboardEvent Data;
	friend class Event;

  public:
	KeyboardEvent(EventTypes type);
	~KeyboardEvent() override = default;
};

class MouseEvent : public Event
{
  private:
	FrameworkMouseEvent Data;
	friend class Event;

  public:
	MouseEvent(EventTypes type);
	~MouseEvent() override = default;
};

class FingerEvent : public Event
{
  private:
	FrameworkFingerEvent Data;
	friend class Event;

  public:
	FingerEvent(EventTypes type);
	~FingerEvent() override = default;
};

class TimerEvent : public Event
{
  private:
	FrameworkTimerEvent Data;
	friend class Event;

  public:
	TimerEvent(EventTypes type);
	~TimerEvent() override = default;
};

class FormsEvent : public Event
{
  private:
	FrameworkFormsEvent Data;
	friend class Event;

  public:
	FormsEvent();
	~FormsEvent() override = default;
};

class TextEvent : public Event
{
  private:
	FrameworkTextEvent Data;
	friend class Event;

  public:
	TextEvent();
	~TextEvent() override = default;
};

class UserEvent : public Event
{
  private:
	FrameworkUserEvent Data;
	friend class Event;

  public:
	UserEvent(const UString &id, sp<void> data = nullptr);
	~UserEvent() override = default;
};

}; // namespace OpenApoc
