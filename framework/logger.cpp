#ifdef _WIN32 // Seems to be set even on win64?
#define BACKTRACE_WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#define ERROR_DIALOG
#endif

#include "framework/logger.h"
#include <chrono>
#include <cstdarg>
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

#ifndef LOGFILE
#define LOGFILE "openapoc_log.txt"
#endif

// Don't have interactive dialogues in unit tests
#ifdef UNIT_TEST
#undef ERROR_DIALOG
#else

#if defined(ERROR_DIALOG)
#include <SDL_messagebox.h>
#endif

#endif

#ifdef ANDROID
#include <android/log.h>
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "OpenApoc", __VA_ARGS__)
#define LOGDV(fmt, ap) __android_log_vprint(ANDROID_LOG_DEBUG, "OpenApoc", fmt, ap)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "OpenApoc", __VA_ARGS__)
#define LOGWV(fmt, ap) __android_log_vprint(ANDROID_LOG_WARN, "OpenApoc", fmt, ap)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OpenApoc", __VA_ARGS__)
#define LOGEV(fmt, ap) __android_log_vprint(ANDROID_LOG_ERROR, "OpenApoc", fmt, ap)
#define LOG_PATH "/storage/emulated/0/openapoc/data/"
#else
#define LOGD(...)
#define LOGDV(fmt, ap)
#define LOGW(...)
#define LOGWV(fmt, ap)
#define LOGE(...)
#define LOGEV(fmt, ap)
#define LOG_PATH
#endif

#define MAX_SYMBOL_LENGTH 1000

namespace OpenApoc
{

#if defined(BACKTRACE_LIBUNWIND)
static void print_backtrace(FILE *f)
{

	unw_cursor_t cursor;
	unw_context_t ctx;
	unw_word_t ip;
	unw_getcontext(&ctx);
	unw_init_local(&cursor, &ctx);

	fprintf(f, "  called by:\n");
	while (unw_step(&cursor) > 0)
	{
		Dl_info info;
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		if (ip == 0)
			break;
		dladdr(reinterpret_cast<void *>(ip), &info);
		if (info.dli_sname)
		{
			fprintf(f, "  0x%lx %s+0x%lx (%s)\n", static_cast<uintptr_t>(ip), info.dli_sname,
			        static_cast<uintptr_t>(ip) - reinterpret_cast<uintptr_t>(info.dli_saddr),
			        info.dli_fname);
			continue;
		}
		// If dladdr() failed, try libunwind
		unw_word_t offsetInFn;
		char fnName[MAX_SYMBOL_LENGTH];
		if (!unw_get_proc_name(&cursor, fnName, MAX_SYMBOL_LENGTH, &offsetInFn))
		{
			fprintf(f, "  0x%lx %s+0x%lx (%s)\n", static_cast<uintptr_t>(ip), fnName, offsetInFn,
			        info.dli_fname);
			continue;
		}
		else
			fprintf(f, "  0x%lx\n", static_cast<uintptr_t>(ip));
	}
}
#elif defined(BACKTRACE_WINDOWS)
#define MAX_STACK_FRAMES 100
static void print_backtrace(FILE *f)
{
	static bool initialised = false;
	static HANDLE process;

	unsigned int frames;
	void *ip[MAX_STACK_FRAMES];
	SYMBOL_INFO *sym;

	if (!initialised)
	{
		process = GetCurrentProcess();
		SymInitialize(process, NULL, true);
		initialised = true;
	}

	if (!process)
	{
		fprintf(f, "Failed to get process for backtrace\n");
		return;
	}

	sym = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + MAX_SYMBOL_LENGTH * (sizeof(char)), 1);
	if (!sym)
	{
		fprintf(f, "Failed to allocate symbol info for backtrace\n");
		return;
	}
	sym->MaxNameLen = MAX_SYMBOL_LENGTH;
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);

	/* Skip 2 frames (print_backtrace and the LogError function itself) */
	frames = CaptureStackBackTrace(2, MAX_STACK_FRAMES, ip, NULL);

	for (unsigned int frame = 0; frame < frames; frame++)
	{
		SymFromAddr(process, (DWORD64)(ip[frame]), 0, sym);
		fprintf(f, "  0x%p %s+0x%lx\n", ip[frame], sym->Name,
		        (uintptr_t)ip[frame] - (uintptr_t)sym->Address);
	}

	free(sym);
}
#else
#warning no backtrace enabled for this platform
static void print_backtrace(FILE *f)
{
	// TODO: Other platform backtrace?
}
#endif

static FILE *outFile = nullptr;

static std::mutex logMutex;
static std::chrono::time_point<std::chrono::high_resolution_clock> timeInit =
    std::chrono::high_resolution_clock::now();

void Log(LogLevel level, UString prefix, UString format, ...)
{
	bool exit_app = false;
	const char *level_prefix;
	auto timeNow = std::chrono::high_resolution_clock::now();
	unsigned long long clockns =
	    std::chrono::duration<unsigned long long, std::nano>(timeNow - timeInit).count();

	logMutex.lock();
	if (!outFile)
	{
#ifdef UNIT_TEST
		outFile = stderr;
#else
#ifndef ANDROID
		outFile = fopen(LOG_PATH LOGFILE, "w");
		if (!outFile)
		{
			// No log file, have to hope stderr goes somewhere useful
			fprintf(stderr, "Failed to open logfile \"%s\"\n", LOGFILE);
			LOGE("Failed to open logfile \"%s\"\n", LOGFILE);
			return;
		}
#endif
#endif
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
#ifdef ANDROID
	LOGD("%s %llu %s: ", level_prefix, clockns, prefix.c_str());
	LOGDV(format.c_str(), arglist);
	va_end(arglist);
#else
	fprintf(outFile, "%s %llu %s: ", level_prefix, clockns, prefix.c_str());
	vfprintf(outFile, format.c_str(), arglist);

	// On error print a backtrace to the log file
	if (level == LogLevel::Error)
		print_backtrace(outFile);
	fprintf(outFile, "\n");
	va_end(arglist);

	// If it's a warning or error flush the outfile (in case we crash 'soon') and also print to
	// stderr
	if (level != LogLevel::Info)
	{
		fflush(outFile);
		va_start(arglist, format);
		fprintf(stderr, "%s %llu %s: ", level_prefix, clockns, prefix.c_str());
		vfprintf(stderr, format.c_str(), arglist);
		fprintf(stderr, "\n");
		va_end(arglist);
	}
#endif
#if defined(ERROR_DIALOG)
	if (level == LogLevel::Error)
	{
		/* How big should the string be? */
		va_start(arglist, format);
		auto strSize = vsnprintf(NULL, 0, format.c_str(), arglist);
		strSize += 1; // NULL terminator
		std::unique_ptr<char[]> string(new char[strSize]);
		va_end(arglist);

		/* Now format the string */
		va_start(arglist, format);
		vsnprintf(string.get(), strSize, format.c_str(), arglist);
		va_end(arglist);

		SDL_MessageBoxData mBoxData;
		mBoxData.flags = SDL_MESSAGEBOX_ERROR;
		mBoxData.window = NULL; // Might happen before we get our window?
		mBoxData.title = "OpenApoc ERROR";
		mBoxData.message = string.get();
		mBoxData.numbuttons = 2;
		SDL_MessageBoxButtonData buttons[2];
		buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		buttons[0].buttonid = 1;
		buttons[0].text = "Exit";
		buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
		buttons[1].buttonid = 2;
		buttons[1].text = "try to limp along";
		mBoxData.buttons = buttons;
		mBoxData.colorScheme = NULL; // Use system settings

		int but;
		SDL_ShowMessageBox(&mBoxData, &but);

		/* button 1 = "exit", button 2 = "try to limp along" */
		if (but == 1)
		{
			exit_app = true;
		}
	}
#endif

	logMutex.unlock();

	if (exit_app)
	{
		exit(EXIT_FAILURE);
	}
}

}; // namespace OpenApoc
