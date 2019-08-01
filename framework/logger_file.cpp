#include "framework/logger_file.h"
#include "framework/configfile.h"
#include "framework/logger.h"

#include <fstream>

namespace OpenApoc
{

namespace
{

ConfigOptionInt fileLogLevelOption(
    "Logger", "FileLevel",
    "Loglevel to output to file (0 = nothing, 1 = error, 2 = warning, 3 = info, 4 = debug)", 3);

LogFunction previousFunction; // To allow chaining log functions

LogLevel fileLogLevel = LogLevel::Nothing;
std::ofstream logFile;

void FileLogFunction(LogLevel level, UString prefix, const UString &text)
{
	previousFunction(level, prefix, text);
	if (level > fileLogLevel)
	{
		return;
	}
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
			return;
		case LogLevel::Debug:
			levelPrefix = "D";
			return;
		default:
			levelPrefix = "U";
			break;
	}
	auto message = OpenApoc::format("%s %s: %s", levelPrefix, prefix, text);
	logFile << message << std::endl;
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
	fileLogLevel = (LogLevel)fileLogLevelOption.get();
	previousFunction = getLogCallback();
	setLogCallback(FileLogFunction);
}

} // namespace OpenApoc
