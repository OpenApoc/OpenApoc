#pragma once

#include "library/strings.h"
#include <boost/locale/format.hpp>
#include <sstream>

namespace OpenApoc
{

template <typename... Args> static inline UString tformat(const UString &string, Args &&... args)
{
	return (boost::locale::format(boost::locale::translate(string)) % ... % args).str();
}

}; // namespace OpenApoc
