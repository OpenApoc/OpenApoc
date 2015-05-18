#ifdef _WIN32 //Seems to be set even on win64?
#define BACKTRACE_WINDOWS
#endif

#include "framework/logger.h"
#include <unicode/ustdio.h>
#include <cstdarg>
#include <mutex>
#include <chrono>
#ifdef BACKTRACE_LIBUNWIND
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#elif defined(BACKTRACE_WINDOWS)
#include <windows.h>
#endif

#ifndef LOGFILE
#define LOGFILE "openapoc_log.txt"
#endif

#define MAX_SYMBOL_LENGTH 1000

namespace OpenApoc {

#if defined(BACKTRACE_LIBUNWIND)
static void print_backtrace(UFILE *f)
{

	unw_cursor_t cursor;
	unw_context_t ctx;
	unw_word_t ip;
	unw_getcontext(&ctx);
	unw_init_local(&cursor, &ctx);

	u_fprintf(f, "  called by:\n");
	while (unw_step(&cursor) > 0)
	{
		Dl_info info;
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		if (ip == 0)
			break;
		dladdr((void*)ip, &info);
		if (info.dli_sname)
		{
			u_fprintf(f, "  0x%p %s+0x%lx (%s)\n", (void*)ip, info.dli_sname, (uintptr_t)ip - (uintptr_t)info.dli_saddr, info.dli_fname);
			continue;
		}
		//If dladdr() failed, try libunwind
		unw_word_t offsetInFn;
		char fnName[MAX_SYMBOL_LENGTH];
		if (!unw_get_proc_name(&cursor, fnName, MAX_SYMBOL_LENGTH, &offsetInFn))
		{
			u_fprintf(f, "  0x%p %s+0x%lx (%s)\n", (void*)ip, fnName, offsetInFn, info.dli_fname);
			continue;
		}
		else
			u_fprintf(f, "  0x%p\n", (void*)ip);
	}
}
#elif defined(BACKTRACE_WINDOWS)
#define MAX_STACK_FRAMES 100
//Stub implementation
//TODO: Implement for windows?
static void print_backtrace(UFILE *f)
{
	static bool initialised = false;
	static HANDLE process;

	unsigned int frame, frames;
	void *ip[MAX_STACK_FRAMES];
	SYMBOL_INFO *sym;

	if (!initialised) {
		process = GetCurrentProcess();
		SymInitialize(procress, NULL, true);
		initialised = true;
	}

	sym = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + MAX_SYMBOL_LENGTH * (sizeof(char)), 1);
	sym->MaxNameLen = MAX_SYMBOL_LENGTH;
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);

	/* Skip 2 frames (print_backtrace and the LogError function itself) */
	frames = CaptureStackBackTrace(2, MAX_STACK_FRAMES, ip, NULL);

	for (unsigned int frame = 0; frame < frames; frame++)
	{
		SymFromAddr(process, (DWORD64)(ip[frame]), 0, sym);
		u_fprintf(f, "  0x%p %s+0x%lx (%s)\n", ip[frame], sym->Name, (uintptr_t)ip[frame] - (uintptr_t)sym->Address);
	}

	free(sym);
}
#else
static void print_backtrace(UFILE *f)
{
	//TODO: Other platform backtrace?
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
