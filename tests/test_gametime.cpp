#include "framework/configfile.h"
#include "framework/logger.h"
#include "game/state/gametime.h"

using namespace OpenApoc;

static bool checkFlags(const GameTime &gt, const char *testName, bool expectSecond,
                       bool expectFiveMinutes, bool expectHour, bool expectDay, bool expectWeek)
{
	bool ok = true;
	if (gt.secondPassed() != expectSecond)
	{
		LogError("{0}: secondPassed() was {1}, expected {2}", testName, gt.secondPassed(),
		         expectSecond);
		ok = false;
	}
	if (gt.fiveMinutesPassed() != expectFiveMinutes)
	{
		LogError("{0}: fiveMinutesPassed() was {1}, expected {2}", testName, gt.fiveMinutesPassed(),
		         expectFiveMinutes);
		ok = false;
	}
	if (gt.hourPassed() != expectHour)
	{
		LogError("{0}: hourPassed() was {1}, expected {2}", testName, gt.hourPassed(), expectHour);
		ok = false;
	}
	if (gt.dayPassed() != expectDay)
	{
		LogError("{0}: dayPassed() was {1}, expected {2}", testName, gt.dayPassed(), expectDay);
		ok = false;
	}
	if (gt.weekPassed() != expectWeek)
	{
		LogError("{0}: weekPassed() was {1}, expected {2}", testName, gt.weekPassed(), expectWeek);
		ok = false;
	}
	return ok;
}

static bool test_within_second_no_flag()
{
	GameTime gt(100);
	gt.addTicks(10);
	return checkFlags(gt, "test_within_second_no_flag", false, false, false, false, false);
}

static bool test_second_boundary()
{
	GameTime gt(TICKS_PER_SECOND - 1);
	gt.addTicks(2);
	return checkFlags(gt, "test_second_boundary", true, false, false, false, false);
}

static bool test_five_minute_boundary()
{
	GameTime gt(5 * TICKS_PER_MINUTE - 1);
	gt.addTicks(2);
	return checkFlags(gt, "test_five_minute_boundary", true, true, false, false, false);
}

static bool test_hour_boundary()
{
	GameTime gt(TICKS_PER_HOUR - 1);
	gt.addTicks(2);
	return checkFlags(gt, "test_hour_boundary", true, true, true, false, false);
}

static bool test_day_boundary()
{
	GameTime gt(TICKS_PER_DAY - 1);
	gt.addTicks(2);
	if (!checkFlags(gt, "test_day_boundary", true, true, true, true, false))
	{
		return false;
	}
	if (gt.getDay() != 2)
	{
		LogError("test_day_boundary: getDay() was {0}, expected 2", gt.getDay());
		return false;
	}
	return true;
}

static bool test_week_boundary_tuesday_start()
{
	GameTime gt(6 * TICKS_PER_DAY - 1);
	gt.addTicks(2);
	if (!checkFlags(gt, "test_week_boundary_tuesday_start", true, true, true, true, true))
	{
		return false;
	}
	if (gt.getDay() != 7)
	{
		LogError("test_week_boundary_tuesday_start: getDay() was {0}, expected 7", gt.getDay());
		return false;
	}
	return true;
}

static bool test_flags_persist_until_cleared()
{
	GameTime gt(TICKS_PER_SECOND - 1);
	gt.addTicks(2);
	if (!checkFlags(gt, "test_flags_persist_until_cleared (before clear)", true, false, false,
	                false, false))
	{
		return false;
	}
	if (!gt.secondPassed())
	{
		LogError("test_flags_persist_until_cleared: secondPassed() did not persist across a "
		         "repeated call before clearFlags()");
		return false;
	}
	gt.clearFlags();
	return checkFlags(gt, "test_flags_persist_until_cleared (after clear)", false, false, false,
	                  false, false);
}

int main(int argc, char **argv)
{
	if (config().parseOptions(argc, argv))
	{
		return EXIT_FAILURE;
	}

	bool allPassed = true;
	allPassed &= test_within_second_no_flag();
	allPassed &= test_second_boundary();
	allPassed &= test_five_minute_boundary();
	allPassed &= test_hour_boundary();
	allPassed &= test_day_boundary();
	allPassed &= test_week_boundary_tuesday_start();
	allPassed &= test_flags_persist_until_cleared();

	if (!allPassed)
	{
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
