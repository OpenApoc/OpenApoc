#pragma once

#include "library/strings.h"
// Include logger for 'LOGGER_PREFIX' definition
#include "framework/logger.h"
#include <vector>

namespace OpenApoc
{

class Trace
{
  public:
	static void enable();
	static void disable();

	static void start(const UString &name,
	                  const std::vector<std::pair<UString, UString>> &args = {});
	static void end(const UString &end);

	static bool enabled;

	static void setThreadName(const UString &name);
};

class TraceObj
{
  public:
	UString name;
	TraceObj(const UString &name, const std::vector<std::pair<UString, UString>> &args = {})
	    : name(name)
	{
		Trace::start(name, args);
	}
	~TraceObj() { Trace::end(name); }
};

#define TRACE_FN TraceObj trace_object_##__COUNTER__(LOGGER_PREFIX)

#define TRACE_FN_ARGS1(a, b) TraceObj trace_object_##__COUNTER__(LOGGER_PREFIX, {{a, b}})

} // namespace OpenApoc
