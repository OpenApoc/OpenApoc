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

template <typename... Args> static inline void LogInfo(OpenApoc::UString format, Args &&... args)
{
	OpenApoc::Log(OpenApoc::LogLevel::Info, LOGGER_PREFIX, format, std::forward<Args>(args)...);
}

template <typename... Args> static inline void LogWarning(OpenApoc::UString format, Args &&... args)
{
	OpenApoc::Log(OpenApoc::LogLevel::Warning, LOGGER_PREFIX, format, std::forward<Args>(args)...);
}

template <typename... Args> static inline void LogError(OpenApoc::UString format, Args &&... args)
{
	OpenApoc::Log(OpenApoc::LogLevel::Error, LOGGER_PREFIX, format, std::forward<Args>(args)...);
}
