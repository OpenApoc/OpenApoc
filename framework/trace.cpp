#include "framework/trace.h"
#include <memory>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>

namespace
{

using OpenApoc::UString;

enum class EventType
{
	Begin,
	End,
};

class TraceEvent
{
  public:
	EventType type;
	UString name;
	std::vector<std::pair<UString, UString>> args;
	uint64_t timeNS;
	TraceEvent(EventType type, const UString &name,
	           const std::vector<std::pair<UString, UString>> &args, uint64_t timeNS)
	    : type(type), name(name), args(args), timeNS(timeNS)
	{
	}
};

class EventList
{
  public:
	UString tid;
	std::vector<TraceEvent> events;
};

class TraceManager
{
  public:
	// We need a list of all the EventLists created for each thread to dump out & free at
	// TraceManager destructor time
	std::list<std::unique_ptr<EventList>> lists;
	std::mutex listMutex;
	EventList *createThreadEventList()
	{
		std::stringstream ss;
		listMutex.lock();
		EventList *list = new EventList;
		ss << std::this_thread::get_id();
		list->tid = ss.str();
		lists.emplace_back(list);
		listMutex.unlock();
		return list;
	}
	~TraceManager();
};

static std::unique_ptr<TraceManager> trace_manager;

// thread_local isn't implemented until msvc 2015 (_MSC_VER 1900)
#if defined(_MSC_VER) && _MSC_VER < 1900
static __declspec(thread) EventList *events = nullptr;
#else
#if defined(BROKEN_THREAD_LOCAL)
#warning Using pthread path
#include <pthread.h>

static pthread_key_t eventListKey;

#else
static thread_local EventList *events = nullptr;
#endif
#endif
static std::chrono::time_point<std::chrono::high_resolution_clock> traceStartTime;

} // anonymous namespace

TraceManager::~TraceManager()
{
	assert(OpenApoc::Trace::enabled);

	std::ofstream outFile("/sdcard/openapoc/data/openapoc_trace.json");

	// FIXME: Use proper json parser instead of magically constructing from strings?

	outFile << "{\"traceEvents\":[\n";

	bool firstEvent = true;

	listMutex.lock();
	for (auto &eventList : lists)
	{
		for (auto &event : eventList->events)
		{
			if (!firstEvent)
				outFile << ",\n";

			firstEvent = false;

			outFile << "{"
			        << "\"pid\":1,"
			        << "\"tid\":\"" << eventList->tid.str() << "\","
			        // Time is in microseconds, not nanoseconds
			        << "\"ts\":" << event.timeNS / 1000 << ","
			        << "\"name\":\"" << event.name.str() << "\",";

			switch (event.type)
			{
				case EventType::Begin:
				{
					outFile << "\"ph\":\"B\","
					        << "\"args\":{";

					bool firstArg = true;

					for (auto &arg : event.args)
					{
						if (!firstArg)
							outFile << ",";
						firstArg = false;
						outFile << "\"" << arg.first.str() << "\":\"" << arg.second.str() << "\"";
					}
					outFile << "}";
					break;
				}
				case EventType::End:
					outFile << "\"ph\":\"E\"";
					break;
			}
			outFile << "}";
		}
	}
	listMutex.unlock();
	outFile << "]}\n";
}

namespace OpenApoc
{

bool Trace::enabled = false;

void Trace::enable()
{
	assert(!trace_manager);
	trace_manager.reset(new TraceManager);
#if defined(BROKEN_THREAD_LOCAL)
	pthread_key_create(&eventListKey, NULL);
#endif
	Trace::enabled = true;
	traceStartTime = std::chrono::high_resolution_clock::now();
}

void Trace::setThreadName(const UString &name)
{
	if (!Trace::enabled)
		return;

#if defined(BROKEN_THREAD_LOCAL)
	EventList *events = (EventList *)pthread_getspecific(eventListKey);
	if (!events)
	{
		events = trace_manager->createThreadEventList();
		pthread_setspecific(eventListKey, events);
	}
#else
	if (!events)
		events = trace_manager->createThreadEventList();
#endif

	events->tid = name;
}

void Trace::start(const UString &name, const std::vector<std::pair<UString, UString>> &args)
{
	if (!Trace::enabled)
		return;
#if defined(BROKEN_THREAD_LOCAL)
	EventList *events = (EventList *)pthread_getspecific(eventListKey);
	if (!events)
	{
		events = trace_manager->createThreadEventList();
		pthread_setspecific(eventListKey, events);
	}
#else
	if (!events)
		events = trace_manager->createThreadEventList();
#endif

	auto timeNow = std::chrono::high_resolution_clock::now();
	uint64_t timeNS = std::chrono::duration<uint64_t, std::nano>(timeNow - traceStartTime).count();
	events->events.emplace_back(EventType::Begin, name, args, timeNS);
}
void Trace::end(const UString &name)
{
	if (!Trace::enabled)
		return;
#if defined(BROKEN_THREAD_LOCAL)
	EventList *events = (EventList *)pthread_getspecific(eventListKey);
	if (!events)
	{
		events = trace_manager->createThreadEventList();
		pthread_setspecific(eventListKey, events);
	}
#else
	if (!events)
		events = trace_manager->createThreadEventList();
#endif
	auto timeNow = std::chrono::high_resolution_clock::now();
	uint64_t timeNS = std::chrono::duration<uint64_t, std::nano>(timeNow - traceStartTime).count();
	events->events.emplace_back(EventType::End, name, std::vector<std::pair<UString, UString>>{},
	                            timeNS);
}

} // namespace OpenApoc
