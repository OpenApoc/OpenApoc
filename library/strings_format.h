#pragma once

#define TINYFORMAT_USE_VARIADIC_TEMPLATES

#ifdef __GNUC__
// Tinyformat has a number of non-annotated switch fallthrough cases
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#include "dependencies/tinyformat/tinyformat.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "library/strings.h"
namespace OpenApoc
{

template <typename... Args> static UString format(const UString &fmt, Args &&... args)
{
	return tfm::format(fmt.cStr(), std::forward<Args>(args)...);
}

UString tr(const UString &str, const UString domain = "ufo_string");

} // namespace OpenApoc
