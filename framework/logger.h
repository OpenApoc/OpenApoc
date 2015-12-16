#pragma once

#include "library/strings.h"

#if defined(_MSC_VER) && _MSC_VER > 1400
#include <sal.h>
#endif

/* The logger is global state as we want it to be available even if the framework hasn't been
 * successfully initialised */

namespace OpenApoc
{
/* MSVC doesn't ahve __PRETTY_FUNCTION__ but __FUNCSIG__? */
// FIXME: !__GNUC__ isn't the same as MSVC
#ifndef __GNUC__
#define LOGGER_PREFIX __FUNCSIG__
#else
#define LOGGER_PREFIX __PRETTY_FUNCTION__
#endif

enum class LogLevel
{
	Info,
	Warning,
	Error,
};
// All format strings (%s) are expected to be UTF8
void Log(LogLevel level, UString prefix, UString format, ...);
// All logger output will be UTF8
}; // namespace OpenApoc

//#ifndef __ANDROID__
#if defined(__GNUC__)
// GCC has an extension if __VA_ARGS__ are not supplied to 'remove' the precending comma
#define LogInfo(f, ...)                                                                            \
	OpenApoc::Log(OpenApoc::LogLevel::Info, OpenApoc::UString(LOGGER_PREFIX), f, ##__VA_ARGS__)
#define LogWarning(f, ...)                                                                         \
	OpenApoc::Log(OpenApoc::LogLevel::Warning, OpenApoc::UString(LOGGER_PREFIX), f, ##__VA_ARGS__)
#define LogError(f, ...)                                                                           \
	OpenApoc::Log(OpenApoc::LogLevel::Error, OpenApoc::UString(LOGGER_PREFIX), f, ##__VA_ARGS__)
#else
// At least msvc automatically removes the comma
#define LogInfo(f, ...) OpenApoc::Log(OpenApoc::LogLevel::Info, LOGGER_PREFIX, f, __VA_ARGS__)
#define LogWarning(f, ...) OpenApoc::Log(OpenApoc::LogLevel::Warning, LOGGER_PREFIX, f, __VA_ARGS__)
#define LogError(f, ...) OpenApoc::Log(OpenApoc::LogLevel::Error, LOGGER_PREFIX, f, __VA_ARGS__)
#endif
//#else
#if 0
#define LogInfo(f, ...)
#define LogWarning(f, ...)
#define LogError(f, ...)
#endif
//#endif