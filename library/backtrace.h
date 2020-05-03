#pragma once

#include "library/sp.h"
#include "library/strings.h"
#include <list>

namespace OpenApoc
{

class backtrace
{
  public:
	virtual ~backtrace() = default;
};

std::ostream &operator<<(std::ostream &lhs, const backtrace &bt);

up<backtrace> new_backtrace();

void debug_trap();

} // namespace OpenApoc
