#include "framework/event.h"

namespace OpenApoc
{

Event::Event(EventTypes type) : eventType(type), Handled(false) {}

// FIXME: Can do validation that 'type' here is sane?
DisplayEvent::DisplayEvent(EventTypes type) : Event(type) {}
JoystickEvent::JoystickEvent(EventTypes type) : Event(type) {}
KeyboardEvent::KeyboardEvent(EventTypes type) : Event(type) {}
MouseEvent::MouseEvent(EventTypes type) : Event(type) {}
FingerEvent::FingerEvent(EventTypes type) : Event(type) {}
TimerEvent::TimerEvent(EventTypes type) : Event(type) {}
FormsEvent::FormsEvent() : Event(EVENT_FORM_INTERACTION) {}
TextEvent::TextEvent() : Event(EVENT_TEXT_INPUT) {}

EventTypes Event::Type() const { return this->eventType; }

// FIXME: Can do validation here that typeof(this) is expected?
FRAMEWORK_DISPLAY_EVENT &Event::Display()
{
	auto *ev = static_cast<DisplayEvent *>(this);
	return ev->Data;
}

FRAMEWORK_JOYSTICK_EVENT &Event::Joystick()
{
	auto *ev = static_cast<JoystickEvent *>(this);
	return ev->Data;
}

FRAMEWORK_KEYBOARD_EVENT &Event::Keyboard()
{
	auto *ev = static_cast<KeyboardEvent *>(this);
	return ev->Data;
}

FRAMEWORK_MOUSE_EVENT &Event::Mouse()
{
	auto *ev = static_cast<MouseEvent *>(this);
	return ev->Data;
}

FRAMEWORK_FINGER_EVENT &Event::Finger()
{
	auto *ev = static_cast<FingerEvent *>(this);
	return ev->Data;
}

FRAMEWORK_TIMER_EVENT &Event::Timer()
{
	auto *ev = static_cast<TimerEvent *>(this);
	return ev->Data;
}

FRAMEWORK_FORMS_EVENT &Event::Forms()
{
	auto *ev = static_cast<FormsEvent *>(this);
	return ev->Data;
}

FRAMEWORK_TEXT_EVENT &Event::Text()
{
	auto *ev = static_cast<TextEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_DISPLAY_EVENT &Event::Display() const
{
	auto *ev = static_cast<const DisplayEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_JOYSTICK_EVENT &Event::Joystick() const
{
	auto *ev = static_cast<const JoystickEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_KEYBOARD_EVENT &Event::Keyboard() const
{
	auto *ev = static_cast<const KeyboardEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_MOUSE_EVENT &Event::Mouse() const
{
	auto *ev = static_cast<const MouseEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_FINGER_EVENT &Event::Finger() const
{
	auto *ev = static_cast<const FingerEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_TIMER_EVENT &Event::Timer() const
{
	auto *ev = static_cast<const TimerEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_FORMS_EVENT &Event::Forms() const
{
	auto *ev = static_cast<const FormsEvent *>(this);
	return ev->Data;
}

const FRAMEWORK_TEXT_EVENT &Event::Text() const
{
	auto *ev = static_cast<const TextEvent *>(this);
	return ev->Data;
}

}; // namespace OpenApoc
