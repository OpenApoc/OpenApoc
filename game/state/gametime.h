#include "library/strings.h"
#include <boost/date_time.hpp>
#include <locale>

namespace OpenApoc
{

static const unsigned TICKS_PER_SECOND = 60;
static const unsigned TICKS_PER_MINUTE = TICKS_PER_SECOND * 60;
static const unsigned TICKS_PER_HOUR = TICKS_PER_MINUTE * 60;
static const unsigned TICKS_PER_DAY = TICKS_PER_HOUR * 24;
static const unsigned TURBO_TICKS = 5 * 60 * TICKS_PER_SECOND;

class GameTime
{
  private:
	static const boost::posix_time::ptime GAME_START;
	static const std::locale TIME_FORMAT, DATE_FORMAT;

	uint64_t ticks;

	bool dayPassedFlag;
	bool weekPassedFlag;

	boost::posix_time::ptime posixTime() const;

  public:
	GameTime(uint64_t ticks) : ticks(ticks), dayPassedFlag(false), weekPassedFlag(false) {}

	void addTicks(uint64_t ticks);

	unsigned int getHours() const;

	unsigned int getMinutes() const;

	unsigned int getDay() const;

	unsigned int getWeek() const;

	uint64_t getTicks() const;

	// returns formatted time in format hh:mm:ss
	UString getTimeString() const;

	// returns formatted date in format a, d m, y
	UString getDateString() const;

	// set at midnight
	bool dayPassed() const;

	// set at sunday midnight
	bool weekPassed() const;

	void clearFlags();

	static GameTime midday();
};
}