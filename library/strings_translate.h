#pragma once

#include "library/strings.h"
#include <boost/locale/format.hpp>
#include <sstream>

namespace OpenApoc
{
template <typename... Args> static inline UString tformat(const UString &string, Args &&... args)
{
	std::ostringstream ss;
	ss << (boost::locale::format(boost::locale::translate(string)) % ... % args);
	return ss.str();
}

}; // namespace OpenApoc
