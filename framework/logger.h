#pragma once

#include "library/strings.h"
#include "library/strings_format.h"

#if defined(_MSC_VER) && _MSC_VER > 1400
#include <sal.h>
#endif

#define XSTR(s) STR(s)
#define STR(s) #s

#if defined(_MSC_VER)
#define NORETURN_FUNCTION __declspec(noreturn)
#else
#define NORETURN_FUNCTION __attribute__((noreturn))
#endif

/* The logger is global state as we want it to be available even if the framework hasn't been
 * successfully initialised */

namespace OpenApoc
{
/* MSVC doesn't ahve __PRETTY_FUNCTION__ but __FUNCSIG__? */
// FIXME: !__GNUC__ isn't the same as MSVC
#ifndef __GNUC__
#define LOGGER_PREFIX __FUNCSIG__
#define PRINTF_ATTR(fmt_string_no, vararg_start_no)
#else
#define LOGGER_PREFIX __PRETTY_FUNCTION__
#define PRINTF_ATTR(fmt_string_no, vararg_start_no)                                                \
	__attribute__((format(printf, fmt_string_no, vararg_start_no)))
#endif

enum class LogLevel : int
{
	Nothing = 0,
	Error = 1,
	Warning = 2,
	Info = 3,
	Debug = 4,
};
void Log(LogLevel level, UString prefix, const UString &text);

NORETURN_FUNCTION void _logAssert(UString prefix, UString string, int line, UString file);

/* Returns if the log level will be output (either to file or stderr or both) */
bool _logLevelEnabled(LogLevel level);
static inline bool logLevelEnabled(LogLevel level)
{
#ifdef NDEBUG
	if (level >= LogLevel::Debug)
		return false;
#endif
	return _logLevelEnabled(level);
}

// All logger output will be UTF8
}; // namespace OpenApoc

#define LogAssert(X)                                                                               \
	do                                                                                             \
	{                                                                                              \
		if (!(X))                                                                                  \
			OpenApoc::_logAssert(LOGGER_PREFIX, STR(X), __LINE__, __FILE__);                       \
	} while (0)

#if defined(__GNUC__)
// GCC has an extension if __VA_ARGS__ are not supplied to 'remove' the precending comma
#define LogDebug(f, ...)                                                                           \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Debug))                                  \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Debug, OpenApoc::UString(LOGGER_PREFIX),             \
			              ::OpenApoc::format(f, ##__VA_ARGS__));                                   \
		}                                                                                          \
	} while (0)
#define LogInfo(f, ...)                                                                            \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Info))                                   \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Info, OpenApoc::UString(LOGGER_PREFIX),              \
			              ::OpenApoc::format(f, ##__VA_ARGS__));                                   \
		}                                                                                          \
	} while (0)
#define LogWarning(f, ...)                                                                         \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Warning))                                \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Warning, OpenApoc::UString(LOGGER_PREFIX),           \
			              ::OpenApoc::format(f, ##__VA_ARGS__));                                   \
		}                                                                                          \
	} while (0)
#define LogError(f, ...)                                                                           \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Error))                                  \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Error, OpenApoc::UString(LOGGER_PREFIX),             \
			              ::OpenApoc::format(f, ##__VA_ARGS__));                                   \
		}                                                                                          \
	} while (0)
#else
// At least msvc automatically removes the comma
#define LogDebug(f, ...)                                                                           \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Debug))                                  \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Debug, LOGGER_PREFIX,                                \
			              ::OpenApoc::format(f, __VA_ARGS__));                                     \
		}                                                                                          \
	} while (0)
#define LogInfo(f, ...)                                                                            \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Info))                                   \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Info, LOGGER_PREFIX,                                 \
			              ::OpenApoc::format(f, __VA_ARGS__));                                     \
		}                                                                                          \
	} while (0)
#define LogWarning(f, ...)                                                                         \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Warning))                                \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Warning, LOGGER_PREFIX,                              \
			              ::OpenApoc::format(f, __VA_ARGS__));                                     \
		}                                                                                          \
	} while (0)
#define LogError(f, ...)                                                                           \
	do                                                                                             \
	{                                                                                              \
		if (OpenApoc::logLevelEnabled(OpenApoc::LogLevel::Error))                                  \
		{                                                                                          \
			OpenApoc::Log(OpenApoc::LogLevel::Error, LOGGER_PREFIX,                                \
			              ::OpenApoc::format(f, __VA_ARGS__));                                     \
		}                                                                                          \
	} while (0)
#endif
