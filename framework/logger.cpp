#ifdef _WIN32
#define BACKTRACE_WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "framework/logger.h"
#include "framework/configfile.h"
#include "framework/framework.h"
#include "library/sp.h"
#include <mutex>
#ifdef BACKTRACE_LIBUNWIND
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#elif defined(BACKTRACE_WINDOWS)
#include <windows.h>
// windows.h must be included before DbgHelp.h
#include <DbgHelp.h>
#endif

#define MAX_SYMBOL_LENGTH 1000

namespace OpenApoc
{
#if defined(BACKTRACE_LIBUNWIND)
std::list<UString> getBacktrace()
{
	unw_cursor_t cursor;
	unw_context_t ctx;
	unw_word_t ip;
	unw_getcontext(&ctx);
	unw_init_local(&cursor, &ctx);

	std::list<UString> backtrace;

	while (unw_step(&cursor) > 0)
	{
		Dl_info info;
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		if (ip == 0)
			break;
		dladdr(reinterpret_cast<void *>(ip), &info);
		if (info.dli_sname)
		{
			backtrace.push_back(
			    format("  0x%zx %s+0x%zx (%s)\n", static_cast<uintptr_t>(ip), info.dli_sname,
			           static_cast<uintptr_t>(ip) - reinterpret_cast<uintptr_t>(info.dli_saddr),
			           info.dli_fname));
			continue;
		}
		// If dladdr() failed, try libunwind
		unw_word_t offsetInFn;
		char fnName[MAX_SYMBOL_LENGTH];
		if (!unw_get_proc_name(&cursor, fnName, MAX_SYMBOL_LENGTH, &offsetInFn))
		{
			backtrace.push_back(format("  0x%zx %s+0x%zx (%s)\n", static_cast<uintptr_t>(ip),
			                           fnName, offsetInFn, info.dli_fname));
			continue;
		}
		else
			backtrace.push_back(format("  0x%zx\n", static_cast<uintptr_t>(ip)));
	}
	return backtrace;
}
#elif defined(BACKTRACE_WINDOWS)
#define MAX_STACK_FRAMES 100
std::list<UString> getBacktrace()
{
	static bool initialised = false;
	static HANDLE process;

	unsigned int frames;
	void *ip[MAX_STACK_FRAMES];
	SYMBOL_INFO *sym;
	std::list<UString> backtrace;

	if (!initialised)
	{
		process = GetCurrentProcess();
		SymInitialize(process, NULL, true);
		initialised = true;
	}

	if (!process)
	{
		return backtrace;
	}

	sym = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + MAX_SYMBOL_LENGTH * (sizeof(char)), 1);
	if (!sym)
	{
		return backtrace;
	}
	sym->MaxNameLen = MAX_SYMBOL_LENGTH;
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);

	/* Skip 1 frame (getBacktrace and the LogError function itself) */
	frames = CaptureStackBackTrace(1, MAX_STACK_FRAMES, ip, NULL);

	for (unsigned int frame = 0; frame < frames; frame++)
	{
		SymFromAddr(process, (DWORD64)(ip[frame]), 0, sym);
		backtrace.push_back(format("  0x%p %s+0x%Ix\n", ip[frame], sym->Name,
		                           (uintptr_t)ip[frame] - (uintptr_t)sym->Address));
	}

	free(sym);
	return backtrace;
}
#else
#warning no backtrace enabled for this platform
std::list<UString> getBacktrace()
{
	// TODO: Other platform backtrace?
	return {};
}
#endif

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
