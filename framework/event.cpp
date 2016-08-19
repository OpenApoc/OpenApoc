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
UserEvent::UserEvent(const UString &id, sp<void> data) : Event(EVENT_USER)
{
	Data.ID = id;
	Data.data = data;
}

EventTypes Event::Type() const { return this->eventType; }

// FIXME: Can do validation here that typeof(this) is expected?
FrameworkDisplayEvent &Event::Display()
{
	auto *ev = static_cast<DisplayEvent *>(this);
	return ev->Data;
}

FrameworkJoystickEvent &Event::Joystick()
{
	auto *ev = static_cast<JoystickEvent *>(this);
	return ev->Data;
}

FrameworkKeyboardEvent &Event::Keyboard()
{
	auto *ev = static_cast<KeyboardEvent *>(this);
	return ev->Data;
}

FrameworkMouseEvent &Event::Mouse()
{
	auto *ev = static_cast<MouseEvent *>(this);
	return ev->Data;
}

FrameworkFingerEvent &Event::Finger()
{
	auto *ev = static_cast<FingerEvent *>(this);
	return ev->Data;
}

FrameworkTimerEvent &Event::Timer()
{
	auto *ev = static_cast<TimerEvent *>(this);
	return ev->Data;
}

FrameworkFormsEvent &Event::Forms()
{
	auto *ev = static_cast<FormsEvent *>(this);
	return ev->Data;
}

FrameworkTextEvent &Event::Text()
{
	auto *ev = static_cast<TextEvent *>(this);
	return ev->Data;
}

FrameworkUserEvent &Event::User()
{
	auto *ev = static_cast<UserEvent *>(this);
	return ev->Data;
}

const FrameworkDisplayEvent &Event::Display() const
{
	auto *ev = static_cast<const DisplayEvent *>(this);
	return ev->Data;
}

const FrameworkJoystickEvent &Event::Joystick() const
{
	auto *ev = static_cast<const JoystickEvent *>(this);
	return ev->Data;
}

const FrameworkKeyboardEvent &Event::Keyboard() const
{
	auto *ev = static_cast<const KeyboardEvent *>(this);
	return ev->Data;
}

const FrameworkMouseEvent &Event::Mouse() const
{
	auto *ev = static_cast<const MouseEvent *>(this);
	return ev->Data;
}

const FrameworkFingerEvent &Event::Finger() const
{
	auto *ev = static_cast<const FingerEvent *>(this);
	return ev->Data;
}

const FrameworkTimerEvent &Event::Timer() const
{
	auto *ev = static_cast<const TimerEvent *>(this);
	return ev->Data;
}

const FrameworkFormsEvent &Event::Forms() const
{
	auto *ev = static_cast<const FormsEvent *>(this);
	return ev->Data;
}

const FrameworkTextEvent &Event::Text() const
{
	auto *ev = static_cast<const TextEvent *>(this);
	return ev->Data;
}
const FrameworkUserEvent &Event::User() const
{
	auto *ev = static_cast<const UserEvent *>(this);
	return ev->Data;
}

}; // namespace OpenApoc
