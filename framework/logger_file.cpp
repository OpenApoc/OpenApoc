#include "framework/logger_file.h"
#include "framework/logger.h"
#include "framework/options.h"

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
	if (level <= fileLogLevel)
	{
		UString levelPrefix;
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
		auto message = OpenApoc::format("%s %s: %s", levelPrefix, prefix, text);
		logFile << message << std::endl;
	}

	if (level <= backtraceLogLevel)
	{
		auto backtrace = getBacktrace();
		for (const auto &frame : backtrace)
		{
			logFile << frame << std::endl;
		}
	}
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
