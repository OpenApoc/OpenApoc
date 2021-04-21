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

		std::vector<std::string> months = {tr("January"), tr("February"), tr("March"),
		                                   tr("April"),   tr("May"),      tr("June"),
		                                   tr("July"),    tr("August"),   tr("September"),
		                                   tr("October"), tr("November"), tr("December")};
		dateFacet->long_month_names(months);

		std::vector<std::string> weekdays = {tr("Sunday"),    tr("Monday"),   tr("Tuesday"),
		                                     tr("Wednesday"), tr("Thursday"), tr("Friday"),
		                                     tr("Saturday")};
		dateFacet->long_weekday_names(weekdays);

		std::vector<std::string> days = {
		    tr("1st"),  tr("2nd"),  tr("3rd"),  tr("4th"),  tr("5th"),  tr("6th"),  tr("7th"),
		    tr("8th"),  tr("9th"),  tr("10th"), tr("11th"), tr("12th"), tr("13th"), tr("14th"),
		    tr("15th"), tr("16th"), tr("17th"), tr("18th"), tr("19th"), tr("20th"), tr("21st"),
		    tr("22nd"), tr("23rd"), tr("24th"), tr("25th"), tr("26th"), tr("27th"), tr("28th"),
		    tr("29th"), tr("30th"), tr("31st")};
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

		std::vector<std::string> months = {tr("January"), tr("February"), tr("March"),
		                                   tr("April"),   tr("May"),      tr("June"),
		                                   tr("July"),    tr("August"),   tr("September"),
		                                   tr("October"), tr("November"), tr("December")};
		dateFacet->long_month_names(months);

		std::vector<std::string> days = {
		    tr("1st"),  tr("2nd"),  tr("3rd"),  tr("4th"),  tr("5th"),  tr("6th"),  tr("7th"),
		    tr("8th"),  tr("9th"),  tr("10th"), tr("11th"), tr("12th"), tr("13th"), tr("14th"),
		    tr("15th"), tr("16th"), tr("17th"), tr("18th"), tr("19th"), tr("20th"), tr("21st"),
		    tr("22nd"), tr("23rd"), tr("24th"), tr("25th"), tr("26th"), tr("27th"), tr("28th"),
		    tr("29th"), tr("30th"), tr("31st")};
		dateFacet->longDayNames(days);
	}
	ss.imbue(*DATE_SHORT_FORMAT);
	ss << getPtime(this->ticks).date();
	return ss.str();
}

UString GameTime::getWeekString() const { return format("%s %d", tr("Week"), getWeek()); }

unsigned int GameTime::getMonth() const
{
	const date currentDate = getPtime(this->ticks).date();

	const int months = (currentDate.year() - GAME_START.date().year()) * 12 + currentDate.month() -
	                   GAME_START.date().month();
	return months;
}

unsigned int GameTime::getWeek() const
{
	const date firstMonday = previous_weekday(GAME_START.date(), greg_weekday(Monday));
	const date lastMonday = previous_weekday(getPtime(this->ticks).date(), greg_weekday(Monday));
	const date_duration duration = lastMonday - firstMonday;
	return duration.days() / 7 + 1;
}

unsigned int GameTime::getFirstDayOfCurrentWeek() const
{
	const date today = getPtime(this->ticks).date();

	// The boost library calculates the first day of the week as Sunday (day_of_week = 0)
	// The game instead consider it as Monday (day_of_week = 1)
	if (today.day_of_week() == 1) // Monday
		return today.year_month_day().day;
	else
	{
		unsigned short days_to_monday = today.day_of_week() - 1;
		if (today.day_of_week() == 0) // Sunday
			days_to_monday = 6;

		const date first_day_of_week = today - days(days_to_monday);
		return first_day_of_week.year_month_day().day;
	}
}

unsigned int GameTime::getLastDayOfCurrentWeek() const
{
	const unsigned short dayOfWeek = getPtime(this->ticks).date().day_of_week();
	unsigned int daysBeforeWeekEnd = 7 - dayOfWeek;
	if (dayOfWeek == 0) // Already sunday
		daysBeforeWeekEnd = 0;

	return getPtime(this->ticks + (daysBeforeWeekEnd * TICKS_PER_DAY)).date().year_month_day().day;
}

unsigned int GameTime::getLastDayOfCurrentMonth() const
{
	return getPtime(this->ticks).date().end_of_month().year_month_day().day;
}

unsigned int GameTime::getDay() const { return (this->ticks + TICKS_PER_DAY - 1) / TICKS_PER_DAY; }

unsigned int GameTime::getMonthDay() const
{
	return getPtime(this->ticks).date().year_month_day().day;
}

unsigned int GameTime::getHours() const { return getPtime(this->ticks).time_of_day().hours(); }

unsigned int GameTime::getMinutes() const { return getPtime(this->ticks).time_of_day().minutes(); }

unsigned int GameTime::getSeconds() const { return getPtime(this->ticks).time_of_day().seconds(); }

unsigned int GameTime::getTicksBetween(unsigned int fromDays, unsigned int fromHours,
                                       unsigned int fromMinutes, unsigned int fromSeconds,
                                       unsigned int toDays, unsigned int toHours,
                                       unsigned int toMinutes, unsigned int toSeconds) const
{
	if (fromDays <= toDays && fromHours <= toHours && fromMinutes <= toMinutes &&
	    fromSeconds < toSeconds)
	{
		unsigned int days_diff_in_ticks = (toDays - fromDays) * TICKS_PER_DAY;
		unsigned int hours_diff_in_ticks = (toHours - fromHours) * TICKS_PER_HOUR;
		unsigned int minutes_diff_in_ticks = (toMinutes - fromMinutes) * TICKS_PER_MINUTE;
		unsigned int seconds_diff_in_ticks = (toSeconds - fromSeconds) * TICKS_PER_SECOND;

		return days_diff_in_ticks + hours_diff_in_ticks + minutes_diff_in_ticks +
		       seconds_diff_in_ticks;
	}
	else
		return 0;
}

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
				// game starts on Tuesday, so week rolls on day 6
				if (days % 7 == 6)
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
