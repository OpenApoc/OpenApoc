#include "game/state/gametime.h"
#include <sstream>

// for my sake
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace OpenApoc
{

const ptime GameTime::GAME_START = ptime(date(2084, Mar, 7), time_duration(0, 0, 0));
// locale controls the facet
const std::locale GameTime::TIME_FORMAT =
    std::locale(std::locale::classic(), new time_facet("%H:%M:%S"));
const std::locale GameTime::DATE_FORMAT =
    std::locale(std::locale::classic(), new date_facet("%A, %d %B, %Y"));

// FIXME: Refactor to always use ptime instead of ticks?
ptime GameTime::posixTime() const
{
	int64_t tickTotal = ticks * (time_duration::ticks_per_second() / TICKS_PER_SECOND);
	return GAME_START + time_duration(0, 0, 0, tickTotal);
}

UString GameTime::getTimeString() const
{
	std::stringstream ss;
	ss.imbue(TIME_FORMAT);
	ss << posixTime();
	return ss.str();
}

UString GameTime::getDateString() const
{
	std::stringstream ss;
	ss.imbue(DATE_FORMAT);
	ss << posixTime();
	return ss.str();
}

unsigned int GameTime::getWeek() const
{
	date_duration duration = posixTime().date() - GAME_START.date();
	return duration.days() / 7;
}

unsigned int GameTime::getDay() const { return posixTime().date().day(); }

unsigned int GameTime::getHours() const { return posixTime().time_of_day().hours(); }

unsigned int GameTime::getMinutes() const { return posixTime().time_of_day().minutes(); }

uint64_t GameTime::getTicks() const { return ticks; }

bool GameTime::dayPassed() const { return dayPassedFlag; }

bool GameTime::weekPassed() const { return weekPassedFlag; }

void GameTime::clearFlags()
{
	dayPassedFlag = false;
	weekPassedFlag = false;
}

void GameTime::addTicks(uint64_t ticks)
{
	this->ticks += ticks;
	uint64_t dayTicks = this->ticks % TICKS_PER_DAY;
	if (dayTicks < ticks)
	{
		uint64_t days = this->ticks / TICKS_PER_DAY;
		dayPassedFlag = true;
		if (days % 7 == 0)
		{
			weekPassedFlag = true;
		}
	}
}

GameTime GameTime::midday() { return GameTime(TICKS_PER_HOUR * 12); }
}