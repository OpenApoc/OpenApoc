#pragma once

#include <boost/locale/format.hpp>
#include <boost/locale/message.hpp>

namespace OpenApoc
{

class UString;

inline auto translate(const UString &str) { return boost::locale::translate(str.str()); }

inline auto tformat(const char *str) { return boost::locale::format(boost::locale::translate(str)); }

// TODO: Pleural message formatting

}; // namespace OpenApoc
