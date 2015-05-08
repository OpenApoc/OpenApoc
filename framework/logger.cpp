#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <mutex>
#include <chrono>

#ifndef LOGFILE
#define LOGFILE "openapoc_log.txt"
#endif

namespace OpenApoc {

static FILE *outFile = nullptr;

static std::mutex logMutex;
static std::chrono::time_point<std::chrono::high_resolution_clock> timeInit = std::chrono::high_resolution_clock::now();

void Log (LogLevel level, const char *prefix, const char* format, ...)
{
	const char *level_prefix;
	auto timeNow = std::chrono::high_resolution_clock::now();
	unsigned long long clockns = std::chrono::duration<unsigned long long, std::nano>(timeNow - timeInit).count();

	logMutex.lock();
	if (!outFile)
	{
		outFile = fopen(LOGFILE, "w");
		if (!outFile)
		{
			//No log file, have to hope stderr goes somewhere useful
			fprintf(stderr, "Failed to open logfile \"%s\"\n", LOGFILE);
			return;
		}
		logMutex.unlock();
		LogInfo("Initialising log");
		logMutex.lock();
	}

	switch (level)
	{
		case LogLevel::Info:
			level_prefix = "I";
			break;
		case LogLevel::Warning:
			level_prefix = "W";
			break;
		default:
			level_prefix = "E";
			break;
	}

	va_list arglist;
	va_start(arglist, format);
	fprintf(outFile, "%s %llu %s: ", level_prefix, clockns, prefix);
	vfprintf(outFile, format, arglist);
	fprintf(outFile, "\n");

	//If it's a warning or error flush the outfile (in case we crash 'soon') and also print to stderr
	if (level != LogLevel::Info)
	{
		fflush(outFile);
		fprintf(stderr, "%s %llu %s: ", level_prefix, clockns, prefix);
		vfprintf(stderr, format, arglist);
		fprintf(stderr, "\n");
	}
	va_end(arglist);

	logMutex.unlock();
}

}; //namespace OpenApoc
