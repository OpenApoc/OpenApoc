#pragma once

#include "library/strings.h"
#include <cstdint>

namespace OpenApoc
{

static const unsigned TICKS_PER_SECOND = 144;
static const unsigned TICKS_PER_MINUTE = TICKS_PER_SECOND * 60;
static const unsigned TICKS_PER_HOUR = TICKS_PER_MINUTE * 60;
static const unsigned TICKS_PER_DAY = TICKS_PER_HOUR * 24;
static const unsigned TURBO_TICKS = 5 * 60 * TICKS_PER_SECOND;

class GameTime
{
  private:
	bool dayPassedFlag = false;
	bool weekPassedFlag = false;

  public:
	uint64_t ticks = 0;
	GameTime() = default;
	GameTime(uint64_t ticks);

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
