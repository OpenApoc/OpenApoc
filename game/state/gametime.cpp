#include "game/state/gametime.h"
#include "game/state/gametime_facet.h"
#include "library/strings_format.h"
#include <locale>
#include <sstream>

#include <boost/date_time.hpp>

// for my sake
using namespace boost::gregorian;
using namespace boost::posix_time;

namespace OpenApoc
{

static const ptime GAME_START = ptime(date(2084, Mar, 7), time_duration(0, 0, 0));
static std::locale *TIME_LONG_FORMAT = nullptr;
static std::locale *TIME_SHORT_FORMAT = nullptr;
static std::locale *DATE_LONG_FORMAT = nullptr;
static std::locale *DATE_SHORT_FORMAT = nullptr;

// FIXME: Refactor to always use ptime instead of ticks?
static time_duration ticksToPosix(int64_t ticks)
{
	int64_t tickTotal = std::round(static_cast<double>(ticks * time_duration::ticks_per_second()) /
	                               TICKS_PER_SECOND);
	return time_duration(0, 0, 0, tickTotal);
}

GameTime::GameTime(uint64_t ticks) : ticks(ticks){};

static boost::posix_time::ptime getPtime(uint64_t ticks)
{
	return GAME_START + ticksToPosix(ticks);
}

UString GameTime::getLongTimeString() const
{
	std::stringstream ss;
	if (TIME_LONG_FORMAT == nullptr)
	{
		// locale controls the facet
		time_facet *timeFacet = new time_facet("%H:%M:%S");
		TIME_LONG_FORMAT = new std::locale(std::locale::classic(), timeFacet);
	}
	ss.imbue(*TIME_LONG_FORMAT);
	ss << getPtime(this->ticks);
	return ss.str();
}

UString GameTime::getShortTimeString() const
{
	std::stringstream ss;
	if (TIME_SHORT_FORMAT == nullptr)
	{
		// locale controls the facet
		time_facet *timeFacet = new time_facet("%H:%M");
		TIME_SHORT_FORMAT = new std::locale(std::locale::classic(), timeFacet);
	}
	ss.imbue(*TIME_SHORT_FORMAT);
	ss << getPtime(this->ticks);
	return ss.str();
}

UString GameTime::getLongDateString() const
{
	std::stringstream ss;
	if (DATE_LONG_FORMAT == nullptr)
	{
		apoc_date_facet *dateFacet = new apoc_date_facet("%A, %E %B, %Y");
		DATE_LONG_FORMAT = new std::locale(std::locale::classic(), dateFacet);

		std::vector<std::string> months = {
		    tr("January").str(), tr("February").str(), tr("March").str(),
		    tr("April").str(),   tr("May").str(),      tr("June").str(),
		    tr("July").str(),    tr("August").str(),   tr("September").str(),
		    tr("October").str(), tr("November").str(), tr("December").str()};
		dateFacet->long_month_names(months);

		std::vector<std::string> weekdays = {
		    tr("Sunday").str(),   tr("Monday").str(), tr("Tuesday").str(), tr("Wednesday").str(),
		    tr("Thursday").str(), tr("Friday").str(), tr("Saturday").str()};
		dateFacet->long_weekday_names(weekdays);

		std::vector<std::string> days = {
		    tr("1st").str(),  tr("2nd").str(),  tr("3rd").str(),  tr("4th").str(),
		    tr("5th").str(),  tr("6th").str(),  tr("7th").str(),  tr("8th").str(),
		    tr("9th").str(),  tr("10th").str(), tr("11th").str(), tr("12th").str(),
		    tr("13th").str(), tr("14th").str(), tr("15th").str(), tr("16th").str(),
		    tr("17th").str(), tr("18th").str(), tr("19th").str(), tr("20th").str(),
		    tr("21st").str(), tr("22nd").str(), tr("23rd").str(), tr("24th").str(),
		    tr("25th").str(), tr("26th").str(), tr("27th").str(), tr("28th").str(),
		    tr("29th").str(), tr("30th").str(), tr("31st").str()};
		dateFacet->longDayNames(days);
	}
	ss.imbue(*DATE_LONG_FORMAT);
	ss << getPtime(this->ticks).date();
	return ss.str();
}

UString GameTime::getShortDateString() const
{
	std::stringstream ss;
	if (DATE_SHORT_FORMAT == nullptr)
	{
		apoc_date_facet *dateFacet = new apoc_date_facet("%E %B, %Y");
		DATE_SHORT_FORMAT = new std::locale(std::locale::classic(), dateFacet);

		std::vector<std::string> months = {
		    tr("January").str(), tr("February").str(), tr("March").str(),
		    tr("April").str(),   tr("May").str(),      tr("June").str(),
		    tr("July").str(),    tr("August").str(),   tr("September").str(),
		    tr("October").str(), tr("November").str(), tr("December").str()};
		dateFacet->long_month_names(months);

		std::vector<std::string> days = {
		    tr("1st").str(),  tr("2nd").str(),  tr("3rd").str(),  tr("4th").str(),
		    tr("5th").str(),  tr("6th").str(),  tr("7th").str(),  tr("8th").str(),
		    tr("9th").str(),  tr("10th").str(), tr("11th").str(), tr("12th").str(),
		    tr("13th").str(), tr("14th").str(), tr("15th").str(), tr("16th").str(),
		    tr("17th").str(), tr("18th").str(), tr("19th").str(), tr("20th").str(),
		    tr("21st").str(), tr("22nd").str(), tr("23rd").str(), tr("24th").str(),
		    tr("25th").str(), tr("26th").str(), tr("27th").str(), tr("28th").str(),
		    tr("29th").str(), tr("30th").str(), tr("31st").str()};
		dateFacet->longDayNames(days);
	}
	ss.imbue(*DATE_SHORT_FORMAT);
	ss << getPtime(this->ticks).date();
	return ss.str();
}

UString GameTime::getWeekString() const { return format("{} {}", tr("Week"), getWeek()); }

unsigned int GameTime::getWeek() const
{
	date firstMonday = previous_weekday(GAME_START.date(), greg_weekday(Monday));
	date lastMonday = previous_weekday(getPtime(this->ticks).date(), greg_weekday(Monday));
	date_duration duration = lastMonday - firstMonday;
	return duration.days() / 7 + 1;
}

unsigned int GameTime::getDay() const { return getPtime(this->ticks).date().day(); }

unsigned int GameTime::getHours() const { return getPtime(this->ticks).time_of_day().hours(); }

unsigned int GameTime::getMinutes() const { return getPtime(this->ticks).time_of_day().minutes(); }

uint64_t GameTime::getTicks() const { return ticks; }

bool GameTime::secondPassed() const { return secondPassedFlag; }

bool GameTime::fiveMinutesPassed() const { return fiveMinutesPassedFlag; }

bool GameTime::hourPassed() const { return hourPassedFlag; }

bool GameTime::dayPassed() const { return dayPassedFlag; }

bool GameTime::weekPassed() const { return weekPassedFlag; }

void GameTime::clearFlags()
{
	secondPassedFlag = false;
	fiveMinutesPassedFlag = false;
	hourPassedFlag = false;
	dayPassedFlag = false;
	weekPassedFlag = false;
}

void GameTime::addTicks(uint64_t ticks)
{
	this->ticks += ticks;
	uint64_t secondTicks = this->ticks % (TICKS_PER_SECOND);
	uint64_t fiveMinutesTicks = this->ticks % (5 * TICKS_PER_MINUTE);
	if (fiveMinutesTicks < ticks)
	{
		secondPassedFlag = true;
		fiveMinutesPassedFlag = true;
		uint64_t hourTicks = this->ticks % TICKS_PER_HOUR;
		if (hourTicks < ticks)
		{
			hourPassedFlag = true;
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
	}
	else
	{
		if (secondTicks < ticks)
		{
			secondPassedFlag = true;
		}
	}
}

GameTime GameTime::midday() { return GameTime(TICKS_PER_HOUR * 12); }
} // namespace OpenApoc
