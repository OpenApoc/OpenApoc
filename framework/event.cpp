
#include "event.h"

namespace OpenApoc {

Event::Event()
{
  Type = EVENT_UNDEFINED;
	Handled = false;
}

Event::~Event()
{

}

}; //namespace OpenApoc
