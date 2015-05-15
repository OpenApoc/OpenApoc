#include "framework/logger.h"
#include <unicode/ustdio.h>
#include <cstdarg>
#include <mutex>
#include <chrono>
#ifdef BACKTRACE_ON_ERROR
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#ifndef LOGFILE
#define LOGFILE "openapoc_log.txt"
#endif


namespace OpenApoc {

#ifdef BACKTRACE_ON_ERROR
static void print_backtrace(UFILE *f)
{
	u_fprintf(f, "  called by:\n");
	unw_cursor_t cursor;
	unw_context_t ctx;
	unw_word_t ip;
	unw_getcontext(&ctx);
	unw_init_local(&cursor, &ctx);
	while (unw_step(&cursor) > 0)
	{
		Dl_info info;
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		dladdr((void*)ip, &info);
		if (info.dli_sname)
			u_fprintf(f, "  0x%p %s+0x%lx (%s)\n", (void*)ip, info.dli_sname, (uintptr_t)ip - (uintptr_t)info.dli_saddr, info.dli_fname);
		else
			u_fprintf(f, "  0x%p\n", (void*)ip);
	}
}
#else
//Stub implementation
//TODO: Implement for windows?
static void print_backtrace(FILE *f)
{

}
#endif


static UFILE *outFile = nullptr;
static UFILE *u_stderr = nullptr;

static std::mutex logMutex;
static std::chrono::time_point<std::chrono::high_resolution_clock> timeInit = std::chrono::high_resolution_clock::now();

void Log (LogLevel level, UString prefix, UString format, ...)
{
	const char *level_prefix;
	auto timeNow = std::chrono::high_resolution_clock::now();
	unsigned long long clockns = std::chrono::duration<unsigned long long, std::nano>(timeNow - timeInit).count();

	logMutex.lock();
	if (!u_stderr)
	{
		u_stderr = u_finit(stdout, NULL, NULL);	
	}
	if (!outFile)
	{
		outFile = u_fopen(LOGFILE, "w", nullptr, nullptr);
		if (!outFile)
		{
			//No log file, have to hope stderr goes somewhere useful
			u_fprintf(u_stderr, "Failed to open logfile \"%s\"\n", LOGFILE);
			return;
		}
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
	u_fprintf(outFile, "%s %llu %S: ", level_prefix, clockns, prefix.getTerminatedBuffer());
	u_vfprintf_u(outFile, format.getTerminatedBuffer(), arglist);
	//On error print a backtrace to the log file
	if (level == LogLevel::Error)
		print_backtrace(outFile);
	u_fprintf(outFile, "\n");
	va_end(arglist);

	//If it's a warning or error flush the outfile (in case we crash 'soon') and also print to stderr
	if (level != LogLevel::Info)
	{
		u_fflush(outFile);
		va_start(arglist, format);
		u_fprintf(u_stderr, "%s %llu %S: ", level_prefix, clockns, prefix.getTerminatedBuffer());
		u_vfprintf_u(u_stderr, format.getTerminatedBuffer(), arglist);
		u_fprintf(u_stderr, "\n");
		va_end(arglist);
	}

	logMutex.unlock();
}

}; //namespace OpenApoc
