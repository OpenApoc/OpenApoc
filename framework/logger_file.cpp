#include "framework/logger_file.h"
#include "framework/logger.h"
#include "framework/options.h"
#include "library/backtrace.h"

#include <fstream>

namespace OpenApoc
{

namespace
{

LogFunction previousFunction; // To allow chaining log functions

LogLevel fileLogLevel = LogLevel::Nothing;
LogLevel backtraceLogLevel = LogLevel::Nothing;
std::ofstream logFile;

void FileLogFunction(LogLevel level, UString prefix, const UString &text)
{
	previousFunction(level, prefix, text);
	auto flush = false;
	if (level <= fileLogLevel)
	{
		UString levelPrefix;
		switch (level)
		{
			case LogLevel::Error:
				levelPrefix = "E";
				flush = true;
				break;
			case LogLevel::Warning:
				levelPrefix = "W";
				flush = true;
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
		const auto message = OpenApoc::format("%s %s: %s", levelPrefix, prefix, text);
		logFile << message << std::endl;
	}

	if (level <= backtraceLogLevel)
	{
		const auto backtrace = new_backtrace();
		logFile << *backtrace << std::endl;
		flush = true;
	}
	if (flush)
		logFile.flush();
}

} // namespace

void enableFileLogger(const char *outputFile)
{
	LogAssert(outputFile);
	logFile.open(outputFile);
	if (!logFile.good())
	{
		LogError("File logger failed to open file \"%s\"", outputFile);
	}
	fileLogLevel = (LogLevel)Options::fileLogLevelOption.get();
	backtraceLogLevel = (LogLevel)Options::backtraceLogLevelOption.get();
	previousFunction = getLogCallback();
	setLogCallback(FileLogFunction);
}

} // namespace OpenApoc
