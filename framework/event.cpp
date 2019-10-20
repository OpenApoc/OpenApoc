#include "framework/event.h"
#include <SDL_mouse.h>

namespace OpenApoc
{
bool Event::isPressed(int mask, MouseButton button)
{
	int bmask = SDL_BUTTON(static_cast<int>(button));
	return (mask & bmask) == bmask;
}

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

EventTypes Event::type() const { return this->eventType; }

// FIXME: Can do validation here that typeof(this) is expected?
FrameworkDisplayEvent &Event::display()
{
	auto *ev = static_cast<DisplayEvent *>(this);
	return ev->Data;
}

FrameworkJoystickEvent &Event::joystick()
{
	auto *ev = static_cast<JoystickEvent *>(this);
	return ev->Data;
}

FrameworkKeyboardEvent &Event::keyboard()
{
	auto *ev = static_cast<KeyboardEvent *>(this);
	return ev->Data;
}

FrameworkMouseEvent &Event::mouse()
{
	auto *ev = static_cast<MouseEvent *>(this);
	return ev->Data;
}

FrameworkFingerEvent &Event::finger()
{
	auto *ev = static_cast<FingerEvent *>(this);
	return ev->Data;
}

FrameworkTimerEvent &Event::timer()
{
	auto *ev = static_cast<TimerEvent *>(this);
	return ev->Data;
}

FrameworkFormsEvent &Event::forms()
{
	auto *ev = static_cast<FormsEvent *>(this);
	return ev->Data;
}

FrameworkTextEvent &Event::text()
{
	auto *ev = static_cast<TextEvent *>(this);
	return ev->Data;
}

FrameworkUserEvent &Event::user()
{
	auto *ev = static_cast<UserEvent *>(this);
	return ev->Data;
}

const FrameworkDisplayEvent &Event::display() const
{
	auto *ev = static_cast<const DisplayEvent *>(this);
	return ev->Data;
}

const FrameworkJoystickEvent &Event::joystick() const
{
	auto *ev = static_cast<const JoystickEvent *>(this);
	return ev->Data;
}

const FrameworkKeyboardEvent &Event::keyboard() const
{
	auto *ev = static_cast<const KeyboardEvent *>(this);
	return ev->Data;
}

const FrameworkMouseEvent &Event::mouse() const
{
	auto *ev = static_cast<const MouseEvent *>(this);
	return ev->Data;
}

const FrameworkFingerEvent &Event::finger() const
{
	auto *ev = static_cast<const FingerEvent *>(this);
	return ev->Data;
}

const FrameworkTimerEvent &Event::timer() const
{
	auto *ev = static_cast<const TimerEvent *>(this);
	return ev->Data;
}

const FrameworkFormsEvent &Event::forms() const
{
	auto *ev = static_cast<const FormsEvent *>(this);
	return ev->Data;
}

const FrameworkTextEvent &Event::text() const
{
	auto *ev = static_cast<const TextEvent *>(this);
	return ev->Data;
}
const FrameworkUserEvent &Event::user() const
{
	auto *ev = static_cast<const UserEvent *>(this);
	return ev->Data;
}

}; // namespace OpenApoc
