#pragma once

#define TINYFORMAT_USE_VARIADIC_TEMPLATES
#include "dependencies/tinyformat/tinyformat.h"
#include "library/strings.h"
namespace OpenApoc
{

template <typename... Args> static UString format(const UString &fmt, Args &&... args)
{
	return tfm::format(fmt.cStr(), std::forward<Args>(args)...);
}

UString tr(const UString &str, const UString domain = "ufo_string");

} // namespace OpenApoc
