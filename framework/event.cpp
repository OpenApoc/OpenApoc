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

Event::~Event() {}

}; // namespace OpenApoc
