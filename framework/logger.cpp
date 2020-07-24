#include "framework/logger.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "library/backtrace.h"
#include "library/sp.h"
#include <iostream>
#include <mutex>

namespace OpenApoc
{

static std::mutex loggerMutex;

void defaultLogFunction(LogLevel level, UString prefix, const UString &text)
{
	UString levelPrefix;
	// Only print Warning/Errors by default
	if (level >= LogLevel::Info)
		return;
	switch (level)
	{
		case LogLevel::Error:
			levelPrefix = "E";
			break;
		case LogLevel::Warning:
			levelPrefix = "W";
			break;
		case LogLevel::Info:
			levelPrefix = "I";
			break;
		case LogLevel::Debug:
			levelPrefix = "D";
			break;
		default:
			levelPrefix = "U";
			break;
	}
	std::cerr << levelPrefix << " " << prefix << " " << text << std::endl;
}

static LogFunction logFunction = defaultLogFunction;

void Log(LogLevel level, UString prefix, const UString &text)
{
	std::lock_guard<std::mutex> lock(loggerMutex);
	logFunction(level, prefix, text);
}

void _logAssert(UString prefix, UString string, int line, UString file)
{
	Log(LogLevel::Error, prefix, format("%s:%d Assertion failed %s", file, line, string));
	debug_trap();
	exit(1);
}

void setLogCallback(LogFunction function)
{
	std::lock_guard<std::mutex> lock(loggerMutex);
	logFunction = function;
}

LogFunction getLogCallback()
{

	std::lock_guard<std::mutex> lock(loggerMutex);
	return logFunction;
}

}; // namespace OpenApoc
