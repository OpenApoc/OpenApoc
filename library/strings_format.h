#pragma once

#include "library/strings.h"
// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/format.hpp>
#include <boost/locale/format.hpp>
namespace OpenApoc
{

static inline boost::format &format(boost::format &f) { return f; }

template <typename T, typename... Args>
static boost::format &format(boost::format &f, T const &arg, Args &&... args)
{
	return format(f % arg, std::forward<Args>(args)...);
}

template <typename... Args> static UString format(const UString &fmt, Args &&... args)
{
	boost::format f(fmt.str());
	return format(f, std::forward<Args>(args)...).str();
}

//_lFormat shouldn't be used directly, instead use OpenApoc::tr()
static inline boost::locale::format &lFormat(boost::locale::format &f) { return f; }

template <typename T, typename... Args>
static boost::locale::format &lFormat(boost::locale::format &f, T const &arg, Args &&... args)
{
	return lFormat(f % arg, std::forward<Args>(args)...);
}

UString tr(const UString &str, const UString domain = "ufo_string");

template <typename... Args> static UString tr(const UString &fmt, Args &&... args)
{
	boost::locale::format f(boost::locale::translate(fmt.str()).str("ufo_string"));
	return lFormat(f, std::forward<Args>(args)...).str();
}

template <typename... Args>
static UString tr(const UString &fmt, const UString domain, Args &&... args)
{
	boost::locale::format f(boost::locale::translate(fmt.str()).str(domain.str()));
	return lFormat(f, std::forward<Args>(args)...).str();
}
} // namespace OpenApoc
