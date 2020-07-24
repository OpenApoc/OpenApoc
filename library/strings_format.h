#pragma once

#include "fmt/printf.h"
#include "library/strings.h"

namespace OpenApoc
{

template <typename... Args> static UString format(const UString &fmt, Args &&... args)
{
	return fmt::sprintf(fmt.str(), std::forward<Args>(args)...);
}

UString tr(const UString &str, const UString domain = "ufo_string");

} // namespace OpenApoc

template <> struct fmt::formatter<OpenApoc::UString> : formatter<std::string>
{
	template <typename FormatContext> auto format(const OpenApoc::UString &s, FormatContext &ctx)
	{
		return formatter<std::string>::format(s.str(), ctx);
	}
};
