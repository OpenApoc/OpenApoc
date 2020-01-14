#pragma once

#define TINYFORMAT_USE_VARIADIC_TEMPLATES

#ifdef __GNUC__
// Tinyformat has a number of non-annotated switch fallthrough cases
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "library/strings.h"
#include "fmt/format.h"
#include "fmt/printf.h"

template <>
struct fmt::formatter<OpenApoc::UString> {
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const OpenApoc::UString& s, FormatContext& ctx) {
	return format_to(ctx.out(), "{}", s.str());
  }
};

namespace OpenApoc
{

UString tr(const UString &str, const UString domain = "ufo_string");
UString tr(const char* str, const UString domain = "ufo_string");

template <typename... Args> static UString format(const UString &fmt, Args &&... args)
{
	return fmt::format(fmt.cStr(), std::forward<Args>(args)...);
}

template <typename... Args> static UString tformat(const char* fmt, Args &&... args)
{
	return fmt::format(tr(fmt).cStr(), std::forward<Args>(args)...);
}

} // namespace OpenApoc
