#include "library/strings.h"

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
	unsigned long ticks;

	bool dayPassedFlag;
	bool weekPassedFlag;

  public:
	GameTime(unsigned long ticks) : ticks(ticks), dayPassedFlag(false), weekPassedFlag(false){};

	void addTicks(unsigned int ticks);

	unsigned int getDayTime() const;

	unsigned int getHours() const;

	unsigned int getMinutes() const;

	unsigned int getDay() const;

	unsigned int getTicks() const;

	// returns formatted time in format hh::mm::ss
	UString getTimeString() const;

	// set at midnight
	bool dayPassed() const;

	// set at sunday midnight
	bool weekPassed() const;

	void clearFlags();

	static GameTime midday();
};
}