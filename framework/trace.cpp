#include "framework/trace.h"
#include "framework/configfile.h"
#include "framework/options.h"
#include <chrono>
#include <fstream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace
{

using OpenApoc::UString;

const unsigned int TRACE_CHUNK_SIZE = 100000;

std::mutex initTraceLock;
static bool traceInited = false;

static void initTrace()
{
	std::lock_guard<std::mutex> l(initTraceLock);
	// Something might have enabled this in the time between check and lock
	if (traceInited)
		return;
	traceInited = true;
	if (OpenApoc::Options::enableTrace.get())
		OpenApoc::Trace::enable();
}

enum class EventType : unsigned char
{
	Begin,
	End,
};

class TraceEvent
{
  public:
	EventType type;
	uint64_t timeNS : 48; // (2^48) nanoseconds = 3.25781223 days
	UString name;
	std::vector<std::pair<UString, UString>> args;
	TraceEvent(EventType type, const UString &name,
	           const std::vector<std::pair<UString, UString>> &args, uint64_t timeNS)
	    : type(type), timeNS(timeNS), name(name), args(args)
	{
	}
	TraceEvent() = default;
};

class EventList
{
  public:
	UString tid;
	std::vector<TraceEvent> *current_buffer;
	void pushEvent(const EventType &type, const UString &name,
	               const std::vector<std::pair<UString, UString>> &args, uint64_t &timeNS)
	{
		if (this->current_buffer->size() == TRACE_CHUNK_SIZE)
		{
			this->buffer_list.emplace_back();
			this->current_buffer = &this->buffer_list.back();
			this->current_buffer->reserve(TRACE_CHUNK_SIZE);
			OpenApoc::TraceObj("Trace::EventList::newbuffer");
		}
		this->current_buffer->emplace_back(type, name, args, timeNS);
	}
	std::list<std::vector<TraceEvent>> buffer_list;

	EventList()
	{
		this->buffer_list.emplace_back();
		this->current_buffer = &buffer_list.back();
		this->current_buffer->reserve(TRACE_CHUNK_SIZE);
	}
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
		auto list = new EventList;
		ss << std::this_thread::get_id();
		list->tid = ss.str();
		lists.emplace_back(list);
		listMutex.unlock();
		return list;
	}
	~TraceManager();
	std::ofstream outFile;
	TraceManager() : outFile(OpenApoc::Options::traceFile.get().str())
	{
		if (!outFile)
		{
			LogError("Failed to open trace file \"%s\"", OpenApoc::Options::traceFile.get());
			return;
		}
	}
	void write();
};

static std::unique_ptr<TraceManager> trace_manager;

#if defined(PTHREADS_AVAILABLE)
#include <pthread.h>
#endif

// thread_local isn't implemented until msvc 2015 (_MSC_VER 1900)
#if defined(_MSC_VER) && _MSC_VER < 1900
static __declspec(thread) EventList *events = nullptr;
#else
#if defined(BROKEN_THREAD_LOCAL)
#warning Using pthread path

static pthread_key_t eventListKey;

#else
static thread_local EventList *events = nullptr;
#endif
#endif
static std::chrono::time_point<std::chrono::high_resolution_clock> traceStartTime;

} // anonymous namespace

TraceManager::~TraceManager()
{
	if (OpenApoc::Trace::enabled)
		this->write();
}

void TraceManager::write()
{
	LogAssert(OpenApoc::Trace::enabled);
	OpenApoc::Trace::enabled = false;

	// FIXME: Use proper json parser instead of magically constructing from strings?

	outFile << "{\"traceEvents\":[\n";

	bool firstEvent = true;

	listMutex.lock();
	for (auto &eventList : lists)
	{
		for (auto &buffer : eventList->buffer_list)
		{
			for (auto &event : buffer)
			{
				if (!firstEvent)
					outFile << ",\n";

				firstEvent = false;

				outFile << "{"
				        << "\"pid\":1,"
				        << "\"tid\":\"" << eventList->tid
				        << "\","
				        // Time is in microseconds, not nanoseconds
				        << "\"ts\":" << event.timeNS / 1000 << ","
				        << "\"name\":\"" << event.name << "\",";

				switch (event.type)
				{
					case EventType::Begin:
					{
						outFile << "\"ph\":\"B\"";

						if (!event.args.empty())
						{
							outFile << ",\"args\":{";

							bool firstArg = true;

							for (auto &arg : event.args)
							{
								if (!firstArg)
									outFile << ",";
								firstArg = false;
								outFile << "\"" << arg.first << "\":\"" << arg.second << "\"";
							}
							outFile << "}";
						}
						break;
					}
					case EventType::End:
						outFile << "\"ph\":\"E\"";
						break;
				}
				outFile << "}";
			}
		}
	}
	listMutex.unlock();
	outFile << "]}\n";
	outFile.flush();
}

namespace OpenApoc
{

bool Trace::enabled = false;

void Trace::enable()
{
	if (!traceInited)
		initTrace();
	LogWarning("Enabling tracing - sizeof(TraceEvent) = %u", (unsigned)sizeof(TraceEvent));
	LogAssert(!trace_manager);
	trace_manager.reset(new TraceManager);
#if defined(BROKEN_THREAD_LOCAL)
	pthread_key_create(&eventListKey, NULL);
#endif
	enabled = true;
	traceStartTime = std::chrono::high_resolution_clock::now();
}

void Trace::disable()
{
	if (!enabled)
		return;
	LogAssert(trace_manager);
	trace_manager->write();
	trace_manager.reset(nullptr);
#if defined(BROKEN_THREAD_LOCAL)
	pthread_key_delete(eventListKey);
#endif
	enabled = false;
}

void Trace::setThreadName(const UString &name)
{
	if (!traceInited)
		initTrace();
#if defined(PTHREADS_AVAILABLE)
#if defined(_GNU_SOURCE)
	pthread_setname_np(pthread_self(), name.cStr());
#elif defined(__APPLE__)
	pthread_setname_np(name.cStr());
#endif
#endif
	if (!enabled)
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
	if (!traceInited)
		initTrace();
	if (!enabled)
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
	events->pushEvent(EventType::Begin, name, args, timeNS);
}
void Trace::end(const UString &name)
{
	if (!enabled)
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
	events->pushEvent(EventType::End, name, std::vector<std::pair<UString, UString>>{}, timeNS);
}

} // namespace OpenApoc
