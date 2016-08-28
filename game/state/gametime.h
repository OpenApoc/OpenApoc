#pragma once

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
	// needs some fancy initialization
	static /*const*/ std::locale *TIME_FORMAT, *DATE_LONG_FORMAT, *DATE_SHORT_FORMAT;

	uint64_t ticks;
	boost::posix_time::ptime datetime;

	bool dayPassedFlag;
	bool weekPassedFlag;

	static boost::posix_time::time_duration ticksToPosix(int64_t ticks);

  public:
	GameTime(uint64_t ticks = 0);

	void addTicks(uint64_t ticks);

	unsigned int getHours() const;

	unsigned int getMinutes() const;

	unsigned int getDay() const;

	unsigned int getWeek() const;

	uint64_t getTicks() const;

	// returns week with prefix
	UString getWeekString() const;

	// returns formatted time in format hh:mm:ss
	UString getTimeString() const;

	// returns formatted date in format a, d m, y
	UString getLongDateString() const;

	// returns formatted date in format d m, y
	UString getShortDateString() const;

	// set at midnight
	bool dayPassed() const;

	// set at sunday midnight
	bool weekPassed() const;

	void clearFlags();

	static GameTime midday();
};
}
