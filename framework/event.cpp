#include "framework/event.h"
#include "framework/logger.h"

#include <sstream>

namespace OpenApoc
{

Event::Event()
{
	memset(this, 0, sizeof(*this));
	Type = EVENT_UNDEFINED;
	Handled = false;
}

Event::Event(const UString &str) : Event()
{
	auto args = str.split(' ');
	if (args.size() < 2 || args[0] != "EVENT")
	{
		LogError("Invalid event string \"%s\"", str.str().c_str());
		return;
	}
	auto type = args[1].split('=');
	if (type.size() != 2)
	{
		LogError("Invalid event string \"%s\" - couldn't parse TYPE", str.str().c_str());
		return;
	}
	if (type[1] == "WINDOW_CLOSED")
	{
		this->Type = EVENT_WINDOW_CLOSED;
	}
	else if (type[1] == "KEY_DOWN")
	{
		this->Type = EVENT_KEY_DOWN;
	}
	else if (type[1] == "KEY_UP")
	{
		this->Type = EVENT_KEY_UP;
	}
	else if (type[1] == "KEY_PRESS")
	{
		this->Type = EVENT_KEY_PRESS;
	}
	else if (type[1] == "MOUSE_MOVE")
	{
		this->Type = EVENT_MOUSE_MOVE;
	}
	else if (type[1] == "MOUSE_DOWN")
	{
		this->Type = EVENT_MOUSE_DOWN;
	}
	else if (type[1] == "MOUSE_UP")
	{
		this->Type = EVENT_MOUSE_UP;
	}
	else if (type[1] == "WINDOW_RESIZE")
	{
		this->Type = EVENT_WINDOW_RESIZE;
	}
	else if (type[1] == "WINDOW_ACTIVATE")
	{
		this->Type = EVENT_WINDOW_ACTIVATE;
	}
	else if (type[1] == "WINDOW_DEACTIVATE")
	{
		this->Type = EVENT_WINDOW_DEACTIVATE;
	}
	else if (type[1] == "END_OF_FRAME")
	{
		this->Type = EVENT_END_OF_FRAME;
	}
	else if (type[1] == "UNDEFINED")
	{
		this->Type = EVENT_UNDEFINED;
	}
	else
	{
		LogError("Invalid event string \"%s\" - unhandled TYPE", str.str().c_str());
		return;
	}

	for (size_t i = 2; i < args.size(); i++)
	{
		auto opts = args[i].split('=');
		if (opts.size() != 2)
		{
			LogError("Invalid event string \"%s\" - couldn't parse argument \"%s\"",
			         str.str().c_str(), args[i].str().c_str());
			return;
		}
		if (!Strings::IsInteger(opts[1]))
		{
			LogError("Invalid event string \"%s\" - couldn't parse option value \"%s\"",
			         str.str().c_str(), args[i].str().c_str());
			return;
		}
		int value = Strings::ToInteger(opts[1]);
		if (opts[0] == "keycode")
		{
			if (this->Type != EVENT_KEY_DOWN && this->Type != EVENT_KEY_UP &&
			    this->Type != EVENT_KEY_PRESS)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Keyboard.KeyCode = value;
		}
		else if (opts[0] == "unichar")
		{
			if (this->Type != EVENT_KEY_DOWN && this->Type != EVENT_KEY_UP &&
			    this->Type != EVENT_KEY_PRESS)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Keyboard.UniChar = value;
		}
		else if (opts[0] == "modifiers")
		{
			if (this->Type != EVENT_KEY_DOWN && this->Type != EVENT_KEY_UP &&
			    this->Type != EVENT_KEY_PRESS)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Keyboard.Modifiers = value;
		}
		else if (opts[0] == "x")
		{
			if (this->Type != EVENT_MOUSE_MOVE && this->Type != EVENT_MOUSE_UP &&
			    this->Type != EVENT_MOUSE_DOWN)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Mouse.X = value;
		}
		else if (opts[0] == "y")
		{
			if (this->Type != EVENT_MOUSE_MOVE && this->Type != EVENT_MOUSE_UP &&
			    this->Type != EVENT_MOUSE_DOWN)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Mouse.Y = value;
		}
		else if (opts[0] == "dx")
		{
			if (this->Type != EVENT_MOUSE_MOVE && this->Type != EVENT_MOUSE_UP &&
			    this->Type != EVENT_MOUSE_DOWN)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Mouse.DeltaX = value;
		}
		else if (opts[0] == "dy")
		{
			if (this->Type != EVENT_MOUSE_MOVE && this->Type != EVENT_MOUSE_UP &&
			    this->Type != EVENT_MOUSE_DOWN)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Mouse.DeltaY = value;
		}
		else if (opts[0] == "dv")
		{
			if (this->Type != EVENT_MOUSE_MOVE && this->Type != EVENT_MOUSE_UP &&
			    this->Type != EVENT_MOUSE_DOWN)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Mouse.WheelVertical = value;
		}
		else if (opts[0] == "dw")
		{
			if (this->Type != EVENT_MOUSE_MOVE && this->Type != EVENT_MOUSE_UP &&
			    this->Type != EVENT_MOUSE_DOWN)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Mouse.WheelHorizontal = value;
		}
		else if (opts[0] == "button")
		{
			if (this->Type != EVENT_MOUSE_MOVE && this->Type != EVENT_MOUSE_UP &&
			    this->Type != EVENT_MOUSE_DOWN)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Mouse.Button = value;
		}
		else if (opts[0] == "w")
		{
			if (this->Type != EVENT_WINDOW_RESIZE)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Display.Width = value;
		}
		else if (opts[0] == "h")
		{
			if (this->Type != EVENT_WINDOW_RESIZE)
			{
				LogError("Invalid event string \"%s\" - option \"%s\" not valid for type",
				         str.str().c_str(), args[i].str().c_str());
				return;
			}
			this->Data.Display.Height = value;
		}
		else
		{
			LogError("Invalid event string \"%s\" - unhandled option \"%s\"", str.str().c_str(),
			         args[i].str().c_str());
			return;
		}
	}
}

UString Event::toString()
{
	std::stringstream ss;
	ss << std::dec;
	ss << "EVENT";
	switch (this->Type)
	{
		case EVENT_WINDOW_CLOSED:
			ss << " type=WINDOW_CLOSED";
			break;
		case EVENT_KEY_DOWN:
			ss << " type=KEY_DOWN";
			ss << " keycode=" << this->Data.Keyboard.KeyCode;
			ss << " unichar=" << this->Data.Keyboard.UniChar;
			ss << " modifiers=" << this->Data.Keyboard.Modifiers;
			break;
		case EVENT_KEY_UP:
			ss << " type=KEY_UP";
			ss << " keycode=" << this->Data.Keyboard.KeyCode;
			ss << " unichar=" << this->Data.Keyboard.UniChar;
			ss << " modifiers=" << this->Data.Keyboard.Modifiers;
			break;
		case EVENT_KEY_PRESS:
			ss << " type=KEY_PRESS";
			ss << " keycode=" << this->Data.Keyboard.KeyCode;
			ss << " unichar=" << this->Data.Keyboard.UniChar;
			ss << " modifiers=" << this->Data.Keyboard.Modifiers;
			break;
		case EVENT_MOUSE_MOVE:
			ss << " type=MOUSE_MOVE";
			ss << " x=" << this->Data.Mouse.X;
			ss << " y=" << this->Data.Mouse.Y;
			ss << " dx=" << this->Data.Mouse.DeltaX;
			ss << " dy=" << this->Data.Mouse.DeltaY;
			ss << " dv=" << this->Data.Mouse.WheelVertical;
			ss << " dw=" << this->Data.Mouse.WheelHorizontal;
			ss << " button=" << this->Data.Mouse.Button;
			break;
		case EVENT_MOUSE_DOWN:
			ss << " type=MOUSE_DOWN";
			ss << " x=" << this->Data.Mouse.X;
			ss << " y=" << this->Data.Mouse.Y;
			ss << " dx=" << this->Data.Mouse.DeltaX;
			ss << " dy=" << this->Data.Mouse.DeltaY;
			ss << " dv=" << this->Data.Mouse.WheelVertical;
			ss << " dw=" << this->Data.Mouse.WheelHorizontal;
			ss << " button=" << this->Data.Mouse.Button;
			break;
		case EVENT_MOUSE_UP:
			ss << " type=MOUSE_UP";
			ss << " x=" << this->Data.Mouse.X;
			ss << " y=" << this->Data.Mouse.Y;
			ss << " dx=" << this->Data.Mouse.DeltaX;
			ss << " dy=" << this->Data.Mouse.DeltaY;
			ss << " dv=" << this->Data.Mouse.WheelVertical;
			ss << " dw=" << this->Data.Mouse.WheelHorizontal;
			ss << " button=" << this->Data.Mouse.Button;
			break;
		case EVENT_WINDOW_RESIZE:
			ss << " type=WINDOW_RESIZE";
			ss << " w=" << this->Data.Display.Width;
			ss << " h=" << this->Data.Display.Height;
			break;
		case EVENT_WINDOW_ACTIVATE:
			ss << " type=WINDOW_ACTIVATE";
			break;
		case EVENT_WINDOW_DEACTIVATE:
			ss << " type=WINDOW_DEACTIVATE";
			break;
		case EVENT_END_OF_FRAME:
			ss << " type=END_OF_FRAME";
			break;
		default:
			LogError("Unhandled event type \"%d\"", this->Type);
			ss << " type=UNDEFINED";
			break;
	}
	return UString(ss.str());
}

Event::~Event() {}

}; // namespace OpenApoc
