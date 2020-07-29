#ifdef _WIN32
#define BACKTRACE_WINDOWS
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define USE_DEBUGBREAK_TRAP
#else
// We assume we're on a posix platform if not windows
#define USE_SIGNAL_TRAP
#include <signal.h>
#endif

#include "library/backtrace.h"
#include "library/strings_format.h"

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

#ifdef USE_DEBUGBREAK_TRAP
void debug_trap() { DebugBreak(); }
#endif

#ifdef USE_SIGNAL_TRAP
void debug_trap() { raise(SIGTRAP); }
#endif

#if defined(BACKTRACE_LIBUNWIND)

class libunwind_backtrace : public backtrace
{
  public:
	std::vector<unw_cursor_t> frames;

	static UString symbolicate(unw_cursor_t ip);

	~libunwind_backtrace() override = default;
};

up<backtrace> new_backtrace()
{

	unw_context_t ctx;
	unw_cursor_t cursor;
	unw_getcontext(&ctx);
	unw_init_local(&cursor, &ctx);
	auto bt = mkup<libunwind_backtrace>();
	while (unw_step(&cursor) > 0)
	{
		bt->frames.push_back(cursor);
	}
	return bt;
}

UString libunwind_backtrace::symbolicate(unw_cursor_t frame)
{
	Dl_info info;
	unw_word_t ip;
	unw_get_reg(&frame, UNW_REG_IP, &ip);
	if (ip == 0)
		return "  (null ip)";
	dladdr(reinterpret_cast<void *>(ip), &info);
	if (info.dli_sname)
	{
		return format("  0x%zx %s+0x%zx (%s)\n", static_cast<uintptr_t>(ip), info.dli_sname,
		              static_cast<uintptr_t>(ip) - reinterpret_cast<uintptr_t>(info.dli_saddr),
		              info.dli_fname);
	}
	// If dladdr() failed, try libunwind
	unw_word_t offsetInFn;
	char fnName[MAX_SYMBOL_LENGTH];
	if (!unw_get_proc_name(&frame, fnName, MAX_SYMBOL_LENGTH, &offsetInFn))
	{
		return format("  0x%zx %s+0x%zx (%s)\n", static_cast<uintptr_t>(ip), fnName, offsetInFn,
		              info.dli_fname);
	}
	else
		return format("  0x%zx\n", static_cast<uintptr_t>(ip));
}

std::ostream &operator<<(std::ostream &lhs, const backtrace &bt)
{
	const auto *unwind_backtrace = dynamic_cast<const libunwind_backtrace *>(&bt);
	if (!unwind_backtrace)
	{
		lhs << "invalid backtrace object";
		return lhs;
	}

	for (const auto &frame : unwind_backtrace->frames)
	{
		lhs << unwind_backtrace->symbolicate(frame) << "\n";
	}
	return lhs;
}

#elif defined(BACKTRACE_WINDOWS)
#define MAX_STACK_FRAMES 100

class win32_backtrace : public backtrace
{
  public:
	std::vector<void *> ip;
	~win32_backtrace() override = default;
	static UString symbolicate(const void *ip);
};

static bool process_initialised = false;
static HANDLE process;

static void init_process()
{
	if (process_initialised)
		return;

	process = GetCurrentProcess();
	SymInitialize(process, NULL, true);
	process_initialised = true;
}

up<backtrace> new_backtrace()
{
	unsigned int frames;
	void *ip[MAX_STACK_FRAMES];

	auto backtrace_object = mkup<win32_backtrace>();

	init_process();

	if (!process)
	{
		return backtrace_object;
	}

	/* Skip 1 frame (the get_backtrace function itself) */
	frames = CaptureStackBackTrace(1, MAX_STACK_FRAMES, ip, NULL);

	for (unsigned int frame = 0; frame < frames; frame++)
	{
		backtrace_object->ip.push_back(ip[frame]);
	}

	return backtrace_object;
}

UString win32_backtrace::symbolicate(const void *ip)
{
	SYMBOL_INFO *sym;
	UString str;

	sym = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + MAX_SYMBOL_LENGTH * (sizeof(char)), 1);
	if (!sym)
	{
		return "";
	}
	sym->MaxNameLen = MAX_SYMBOL_LENGTH;
	sym->SizeOfStruct = sizeof(SYMBOL_INFO);

	SymFromAddr(process, (DWORD64)(ip), 0, sym);
	str = format("  0x%p %s+0x%x\n", ip, sym->Name, (uintptr_t)ip - (uintptr_t)sym->Address);

	free(sym);
	return str;
}

std::ostream &operator<<(std::ostream &lhs, const backtrace &bt)
{
	const auto *backtrace_object = dynamic_cast<const win32_backtrace *>(&bt);
	if (!backtrace_object)
	{
		lhs << "invalid backtrace object";
		return lhs;
	}

	for (const auto &frame : backtrace_object->ip)
	{
		lhs << backtrace_object->symbolicate(frame) << "\n";
	}
	return lhs;
}
#else
#warning no backtrace enabled for this platform
class null_backtrace : public backtrace
{
  public:
	~null_backtrace() override = default;
};

up<backtrace> new_backtrace()
{
	// TODO: Other platform backtrace?
	return mkup<null_backtrace>();
}

std::ostream &operator<<(std::ostream &lhs, const backtrace &bt)
{
	const auto *backtrace_object = dynamic_cast<const null_backtrace *>(&bt);
	if (!backtrace_object)
	{
		lhs << "invalid backtrace object";
		return lhs;
	}

	lhs << "backtrace disabled";
	return lhs;
}

#endif

} // namespace OpenApoc
